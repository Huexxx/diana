/* Copyright (C) 2010 emsys Embedded Systems GmbH
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "p_communications.h"
#include "personality.h"

struct mock_function {
  struct kobject kobj;
  struct usb_configuration config;
  struct usb_gadget *gadget; /* for logging macros */
};

static struct mock_function* kobject_to_mock_function(struct kobject* kobj)
{
  return kobj ? container_of(kobj, struct mock_function, kobj) : 0;
}

static struct mock_function* config_to_mock_function(struct usb_configuration* config)
{
  return config ? container_of(config, struct mock_function, config) : 0;
}

static void mock_function_release(struct kobject* kobj)
{
  DBG(kobject_to_mock_function(kobj), "mock_function_release\n");
  kfree(kobject_to_mock_function(kobj));
}

static struct kobj_type comm_func_type = { mock_function_release, &kobj_sysfs_ops };

static int mock_config_bind(struct usb_configuration* config)
{
  if (config) {
    int error = kobject_uevent(&config_to_mock_function(config)->kobj, KOBJ_ADD);
    if (error) {
      DBG(config_to_mock_function(config), "kobject_uevent -> %d\n", error);
      if (-ENOENT == error) {
        /* "No such file or directory" : Most likely the kernel was
           configured to call /sbin/hotplug, which isn't available on
           android systems. Ignore this error. */
        error = 0;
      }
    }
    return error;
  } else {
    return -EINVAL;
  }
}

static void mock_config_unbind(struct usb_configuration* config)
{
  if (config) {
    DBG(config_to_mock_function(config), "mock_config_unbind\n");
    kobject_put(&config_to_mock_function(config)->kobj);
  }
}

void add_mock_configurations(struct usb_composite_dev* cdev, const char* name, const char* label)
{
  struct kobject * parent = &cdev->gadget->dev.kobj;
  struct mock_function * com_func = kzalloc(sizeof(*com_func), GFP_KERNEL);
  DBG(cdev, "add_mock_configurations\n");
  if (!com_func) {
    WARNING(cdev, "out of memory\n");
    return;
  }
  com_func->gadget = cdev->gadget;
  com_func->kobj.kset = personality_cdev_to_kset(cdev);
  if (0 == kobject_init_and_add(&com_func->kobj, &comm_func_type, parent, name)) {
    com_func->config.bind = mock_config_bind;
    com_func->config.unbind = mock_config_unbind;
    com_func->config.bConfigurationValue = 1;
    com_func->config.label = name;
    usb_add_config(cdev, &com_func->config);
  } else {
    WARNING(cdev, "add_mock_configurations failed\n");
    kobject_put(&com_func->kobj);
  }
}

void switch_to_communications_processor(struct usb_composite_dev* cdev, const char* label)
{
  add_mock_configurations(cdev, "communications", label);
}

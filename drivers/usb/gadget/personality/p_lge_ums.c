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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/usb/ch9.h>

#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>

#include "disable_init_defines.h"

#include "p_adb.h"
#include "p_msc.h"
#include "personality.h"


static int lge_ums_bind(struct usb_configuration *c)
{
  int ret = 0;
  struct usb_device_descriptor* device_descriptor = personality_cdev_to_device_desc(c->cdev);

  printk(KERN_INFO "%s(%p)\n", __func__, c);

  if (!device_descriptor) {
    return -EINVAL;
  }

  ret = mass_storage_function_add(c->cdev, c);
  if (ret)
    return ret;
  device_descriptor->bDeviceClass = 2;
  device_descriptor->idProduct = 0x6181;
  return 0;
}

static int lge_ums_adb_bind(struct usb_configuration *c)
{
  int ret = lge_ums_bind(c);
  if (ret) {
    return ret;
  }
  return adb_function_add(c->cdev, c);
}

static void lge_ums_unbind(struct usb_configuration *c)
{
  printk(KERN_INFO "%s\n", __func__);
  personality_cdev_to_device_desc(c->cdev)->bDeviceClass = 0;
}

void add_lge_ums_adb_configurations(struct usb_composite_dev* cdev, const char* name)
{
  static struct usb_configuration config = {
    .bind = lge_ums_adb_bind,
    .unbind = lge_ums_unbind,
    .bConfigurationValue = 1,
    .bmAttributes = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
  };
  printk(KERN_INFO "%s\n", __func__);
  config.label = name;
  usb_add_config(cdev, &config);
}

void add_lge_ums_configurations(struct usb_composite_dev* cdev, const char* name)
{
  static struct usb_configuration config = {
    .bind = lge_ums_bind,
    .unbind = lge_ums_unbind,
    .bConfigurationValue = 1,
    .bmAttributes = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
  };
  printk(KERN_INFO "%s\n", __func__);
  config.label = name;
  usb_add_config(cdev, &config);
}

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
#include "p_serial.h"
#include "personality.h"

#include "../u_serial.c" /* for gserial_setup */

static int lge_normal_bind(struct usb_configuration *c)
{
  int ret = 0;
/* LGE_CHANGE_S kenneth.kang@lge.com 2011-01-16 USB patch */
  struct usb_device_descriptor* device_descriptor = personality_cdev_to_device_desc(c->cdev);
  printk(KERN_INFO "%s(%p)\n", __func__, c);
  if (!device_descriptor) {
    return -EINVAL;
  }
/* LGE_CHANGE_E kenneth.kang@lge.com 2011-01-16 USB patch */  
  ret = acm_function_add(c, 0);        /* bound to /dev/ttyGS0 */
  if (ret)
    return ret;
  ret = gser_function_add(c, 1);       /* bound to /dev/ttyGS1 */
  if (ret)
    return ret;
  ret = gser_function_add(c, 2);       /* bound to /dev/ttyGS2 */
  if (ret)
    return ret;
  gserial_setup(c->cdev->gadget, 3);  /* create N new tty's */
  ret =  mass_storage_function_add(c->cdev, c);
  if (ret)
    return ret;
/* LGE_CHANGE_S kenneth.kang@lge.com 2011-01-16 USB patch */    
  device_descriptor->bDeviceClass = 2;
  device_descriptor->idProduct = 0x618e;
  return 0;
}
static int lge_normal_adb_bind(struct usb_configuration *c)
{
  int ret = lge_normal_bind(c);
  if (ret) {
    return ret;
  }
/* LGE_CHANGE_E kenneth.kang@lge.com 2011-01-16 USB patch */  
  return adb_function_add(c->cdev, c);
}

static void lge_normal_unbind(struct usb_configuration *c)
{
  printk(KERN_INFO "%s\n", __func__);
  personality_cdev_to_device_desc(c->cdev)->bDeviceClass = 0;  
  gserial_cleanup();
}
/* LGE_CHANGE_S kenneth.kang@lge.com 2011-01-16 USB patch */
void add_lge_normal_adb_configurations(struct usb_composite_dev* cdev, const char* name)
{
  static struct usb_configuration config = {
    .bind = lge_normal_adb_bind,
    .unbind = lge_normal_unbind,
    .bConfigurationValue = 1,
    .bmAttributes = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
  };
  printk(KERN_INFO "%s\n", __func__);
  config.label = name;
  usb_add_config(cdev, &config);
}
/* LGE_CHANGE_E kenneth.kang@lge.com 2011-01-16 USB patch */
void add_lge_normal_configurations(struct usb_composite_dev* cdev, const char* name)
{
  static struct usb_configuration config = {
    .bind   = lge_normal_bind,
    .unbind   = lge_normal_unbind,
    .bConfigurationValue = 1,
    .bmAttributes = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
  };
  printk(KERN_INFO "%s\n", __func__);
  config.label = name;
  usb_add_config(cdev, &config);
}


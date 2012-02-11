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
#include "p_msc.h"
#include "personality.h"

static int lge_vtp_bind(struct usb_configuration *c)
{
  printk(KERN_INFO "%s(%p)\n", __func__, c);
/* LGE_CHANGE_S kenneth.kang@lge.com 2011-01-16 USB patch */  
  personality_cdev_to_device_desc(c->cdev)->idProduct = 0x61d4;
/* LGE_CHANGE_E kenneth.kang@lge.com 2011-01-16 USB patch */  
  return cdrom_function_add(c->cdev, c);
}

void add_lge_vtp_configurations(struct usb_composite_dev* cdev, const char* name)
{
  static struct usb_configuration config = {
    .bind   = lge_vtp_bind,
    .bConfigurationValue = 1,
    .bmAttributes = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
  };
  printk(KERN_INFO "%s\n", __func__);
  config.label = name;
  usb_add_config(cdev, &config);
}


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
#include "p_adb.h"
#include "p_mtp.h"

static int  mtp_config_bind(struct usb_configuration *c)
{
  return mtp_function_add(c->cdev, c);
}
static int  mtp_adb_config_bind(struct usb_configuration *c)
{
  int result = mtp_function_add(c->cdev, c);
  if (!result) {
    result = adb_function_add(c->cdev, c);
  }
  return result;
}

static int  dps_config_bind(struct usb_configuration *c)
{
  return dps_function_add(c->cdev, c);
}
static void dps_config_unbind(struct usb_configuration *c)
{
}

void add_dps_configurations(struct usb_composite_dev* cdev, const char* name)
{
  static struct usb_configuration config = {
    .bind		= dps_config_bind,
    .unbind		= dps_config_unbind,
    .bConfigurationValue = 1,
    .bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
  };
  config.label = name;
  usb_add_config(cdev, &config);
}
/* void add_dps_adb_configurations(struct usb_composite_dev* cdev, const char* name) */
/* { */
/*   static struct usb_configuration config = { */
/*     .bind		= mtp_adb_config_bind, */
/*     .bConfigurationValue = 1, */
/*     .bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER, */
/*   }; */
/*   config.label = name; */
/*   usb_add_config(cdev, &config); */
/* } */
void add_mtp_configurations(struct usb_composite_dev* cdev, const char* name)
{
  static struct usb_configuration config = {
    .bind		= mtp_config_bind,
    .setup		= send_mtp_extended_compat_id,
    .bConfigurationValue = 1,
    .bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
  };
  config.label = name;
  usb_add_config(cdev, &config);
}
void add_mtp_adb_configurations(struct usb_composite_dev* cdev, const char* name)
{
  static struct usb_configuration config = {
    .bind		= mtp_adb_config_bind,
    .setup		= send_mtp_none_extended_compat_id,
    .bConfigurationValue = 1,
    .bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
  };
  config.label = name;
  usb_add_config(cdev, &config);
}

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
#ifndef DRIVERS_USB_GADGET_PERSONALITY_PMTP_H
#define DRIVERS_USB_GADGET_PERSONALITY_PMTP_H

#include "linux/device.h"
#include "linux/errno.h"
#include "linux/list.h"
#include "linux/usb/composite.h"
/* LGE_CHANGE_S kenneth.kang@lge.com 2011-01-16 USB patch */
int dps_function_add(struct usb_composite_dev*, struct usb_configuration*);
int mtp_function_add(struct usb_composite_dev*, struct usb_configuration*);
/* LGE_CHANGE_E kenneth.kang@lge.com 2011-01-16 USB patch */
int send_mtp_extended_compat_id(struct usb_configuration*, const struct usb_ctrlrequest*);
int send_mtp_none_extended_compat_id(struct usb_configuration*, const struct usb_ctrlrequest*);

#endif /* ndef DRIVERS_USB_GADGET_PERSONALITY_PMTP_H */

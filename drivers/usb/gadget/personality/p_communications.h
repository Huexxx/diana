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
#ifndef DRIVERS_USB_GADGET_PERSONALITY_PCOMMUNICATIONS_H
#define DRIVERS_USB_GADGET_PERSONALITY_PCOMMUNICATIONS_H

#include "linux/device.h"
#include "linux/errno.h"
#include "linux/list.h"
#include "linux/usb/composite.h"

void switch_to_communications_processor(struct usb_composite_dev* cdev, const char* name);

#endif /* ndef DRIVERS_USB_GADGET_PERSONALITY_PCOMMUNICATIONS_H */

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

#ifndef DRIVERS_USB_GADGET_LGE_NORMAL_PERSONALITY_H
#define DRIVERS_USB_GADGET_LGE_NORMAL_PERSONALITY_H
/* LGE_CHANGE_S kenneth.kang@lge.com 2011-01-16 USB patch */
void add_lge_normal_adb_configurations(struct usb_composite_dev*, const char*);
/* LGE_CHANGE_E kenneth.kang@lge.com 2011-01-16 USB patch */
void add_lge_normal_configurations(struct usb_composite_dev*, const char*);

#endif /* DRIVERS_USB_GADGET_LGE_NORMAL_PERSONALITY_H */


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
/* MediaTransfer, DirectPrint and AndroidDebug personality support */

#ifndef DRIVERS_USB_GADGET_MTPADBPERSONALITY_H
#define DRIVERS_USB_GADGET_MTPADBPERSONALITY_H

void add_dps_configurations(struct usb_composite_dev* cdev, const char* name);
void add_dps_adb_configurations(struct usb_composite_dev* cdev, const char* name);
void add_mtp_configurations(struct usb_composite_dev* cdev, const char* name);
void add_mtp_adb_configurations(struct usb_composite_dev* cdev, const char* name);

#endif /* ndef DRIVERS_USB_GADGET_MTPADBPERSONALITY_H */

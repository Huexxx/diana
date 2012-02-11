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
#ifndef DRIVERS_USB_GADGET_PERSONALITY_PERSONALITY_H
#define DRIVERS_USB_GADGET_PERSONALITY_PERSONALITY_H

enum {
/* LGE_CHANGE_S kenneth.kang@lge.com 2011-01-16 USB patch */
  USB_CDROM_FOR_PCSUITE = 31,
  USB_CDROM_FOR_CP = 32,
/* LGE_CHANGE_S kenneth.kang@lge.com 2011-01-16 USB patch */  
  /* StringDescriptorIds */
  USB_PERSONALITY_OS_STRING_DESCRIPTOR = 0xee,
  USB_PERSONALITY_MANUFACTURER = 0x51,
  USB_PERSONALITY_PRODUCT,
  USB_PERSONALITY_SERIAL_NUMBER,
  USB_PERSONALITY_INTERFACE_ADB,
  USB_PERSONALITY_INTERFACE_CDC,
  USB_PERSONALITY_INTERFACE_DPS,
  USB_PERSONALITY_INTERFACE_MTP,
  /* RequestIds */
  USB_PERSONALITY_GET_MICROSOFT_OS_FEATURE_DESCRIPTOR_REQUEST = 0xFE,
  /* This number is also hardcoded in the MSFT string. */
};

struct kset * personality_cdev_to_kset(struct usb_composite_dev* cdev);
struct usb_device_descriptor* personality_cdev_to_device_desc(struct usb_composite_dev* cdev);
#if 0
#define USB_PERSONALITY_ID_LG_PCSUITE_CDROM    0x0001
#define USB_PERSONALITY_ID_LG_PCINTERNET_CDROM 0x0002
#define USB_PERSONALITY_ID_LG_PCSUITE          0x618e
#define USB_PERSONALITY_ID_LG_PCINTERNET       0x4350
#endif
/**
 * Returns the id of the currently selected USB personality.

 * @param cdev pointer to a composite device representing a
 *             personality gardet.
 * @return personality id
 */
extern int get_personality(struct usb_composite_dev* cdev);

/**
 * Changes the next personality. The device will re-enumerate to
 * this personality immediately (if soft-reconnect is supported)
 * or at the next connect (initiated by the user).
 * Note: The actual switch is deferred and doesn't happen in the
 * context of the caller, because that may take some time, so this
 * may be called from any context.

 * @param cdev pointer to a composite device representing a
 *             personality gardet.
 * @param next personality id
 * @return zero or negative error code
 */
extern int set_next_personality(struct usb_composite_dev* cdev, int next);

#endif /* ndef DRIVERS_USB_GADGET_PERSONALITY_PERSONALITY_H */

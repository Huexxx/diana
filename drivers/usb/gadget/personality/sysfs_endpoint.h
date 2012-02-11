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
#ifndef DRIVERS_USB_GADGET_PERSONALITY_SYSFSENDPOINT_H
#define DRIVERS_USB_GADGET_PERSONALITY_SYSFSENDPOINT_H

#include "linux/device.h"
#include "linux/usb/ch9.h"
#include "linux/sched.h"
#include "linux/usb/gadget.h"
#include "linux/wait.h"

struct sysfs_endpoint {
  struct usb_ep* ep;
  struct usb_gadget* gadget; /* Just for logging. */
  struct usb_request* request;
  struct semaphore lock;
  struct completion done;
  wait_queue_head_t waitq;
  struct usb_endpoint_descriptor* bound_desc;   /* Used for endpoint allocation. */
  struct usb_endpoint_descriptor* enabled_desc; /* Used for current max packet size. */
};
typedef struct sysfs_endpoint sysfs_endpoint;

ssize_t sysfs_endpoint_read(sysfs_endpoint* sys_ep, char *buffer, size_t size);
ssize_t sysfs_endpoint_write(sysfs_endpoint* sys_ep, char *buffer, size_t size);
ssize_t sysfs_endpoint_stall(sysfs_endpoint* sys_ep);
int sysfs_endpoint_enable(sysfs_endpoint* sys_ep, struct usb_endpoint_descriptor* desc);
int sysfs_endpoint_disable(sysfs_endpoint* sys_ep);
int sysfs_endpoint_bind(sysfs_endpoint* sys_ep, struct usb_endpoint_descriptor* desc,
                        struct usb_gadget* gadget, void* gadget_ep_driver_data);
int sysfs_endpoint_unbind(sysfs_endpoint* sys_ep);

#endif /* ndef DRIVERS_USB_GADGET_PERSONALITY_SYSFSENDPOINT_H */

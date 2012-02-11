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
#include "sysfs_endpoint.h"
#include "linux/usb/composite.h"

static void sysfs_complete(struct usb_ep *ep, struct usb_request *usb_req)
{
  sysfs_endpoint* sys_ep = (sysfs_endpoint*)usb_req->context;
  /* DBG(sys_ep, "%s(%p)\n", __func__, ep); */
  complete(&sys_ep->done);
}
static ssize_t read_or_write_locked(sysfs_endpoint* sys_ep, char *buffer, size_t size, bool read)
{
  if (wait_event_interruptible(sys_ep->waitq, !sys_ep->bound_desc || sys_ep->enabled_desc)) {
    return -EINTR;
  } else if (!sys_ep->bound_desc) { /* The usb function is being removed */
    return -ESHUTDOWN;
  } else if (!sys_ep->enabled_desc) { /* Bug in wait() function? */
    return -EAGAIN; 
  } else {
    int result;
    struct usb_request* request = sys_ep->request;
    request->buf = buffer;
    if (read) {
      request->length = size;
    } else if (size < PAGE_SIZE) {
      request->length = size-1;
    } else {
      request->length = PAGE_SIZE - 512;
    }
    result = usb_ep_queue(sys_ep->ep, request, GFP_KERNEL);
    if (result) {
      WARNING(sys_ep, "%s: usb_ep_queue failed (0x%08x)\n", __func__, result);
      return result;
    }
    if (wait_for_completion_interruptible(&sys_ep->done)) {
      return -EINTR;
    }
    return request->status ? request->status : request->actual;
  }
}
static ssize_t read_or_write(sysfs_endpoint* sys_ep, char *buffer, size_t size, bool read)
{
  if (!size) {
    return 0;
  } else if (!sys_ep || !buffer) {
    return -EINVAL;
  }
  DBG(sys_ep, "%s(... size=%d, read=%d)\n", __func__, size, read);
  if (!down_interruptible(&sys_ep->lock)) {
    ssize_t result = read_or_write_locked(sys_ep, buffer, size, read);
    up(&sys_ep->lock);
    return result;
  } else {
    return -EINTR;
  }
}

ssize_t sysfs_endpoint_read(sysfs_endpoint* sys_ep, char *buffer, size_t size)
{
  return read_or_write(sys_ep, buffer, size, /* read = */ true);
}
ssize_t sysfs_endpoint_write(sysfs_endpoint* sys_ep, char *buffer, size_t size)
{
  return read_or_write(sys_ep, buffer, size, /* read = */ false);
}
ssize_t sysfs_endpoint_stall(sysfs_endpoint* sys_ep)
{
  return -ENOTSUPP;
}
int sysfs_endpoint_enable(sysfs_endpoint* sys_ep, struct usb_endpoint_descriptor* enable_desc)
{
  struct usb_request* request = 0;
  if (!sys_ep) {
    return -EINVAL;
  }
  DBG(sys_ep, "%s\n", __func__);
  if (sys_ep->enabled_desc) {
    return 0;
  }
  if (usb_ep_enable(sys_ep->ep, enable_desc)) {
    return -ENODEV;
  }
  request = usb_ep_alloc_request(sys_ep->ep, GFP_KERNEL);
  if (!request) {
    usb_ep_disable(sys_ep->ep);
    return -ENOMEM;
  }
  request->complete = sysfs_complete;
  request->context = sys_ep;
  sys_ep->request = request;
  sys_ep->enabled_desc = enable_desc;
  wake_up_interruptible(&sys_ep->waitq);
  return 0;
}
int sysfs_endpoint_disable(sysfs_endpoint* sys_ep)
{
  int error = 0;
  if (!sys_ep) {
    return -EINVAL;
  }
  DBG(sys_ep, "%s\n", __func__);
  if (!sys_ep->enabled_desc) {
    return 0;
  }
  error = usb_ep_disable(sys_ep->ep);
  sys_ep->enabled_desc = 0;
  if (sys_ep->request) { /* This call will flag the request as "dead" if it's still in use. */
    usb_ep_free_request(sys_ep->ep, sys_ep->request); 
  }
  return error;
}
int sysfs_endpoint_bind(sysfs_endpoint* sys_ep, struct usb_endpoint_descriptor* bind_desc,
                        struct usb_gadget* gadget, void* gadget_ep_driver_data)
{
  if (sys_ep && bind_desc && gadget) {
    sys_ep->gadget = gadget;
    DBG(sys_ep, "%s\n", __func__);
    init_waitqueue_head(&sys_ep->waitq);
    init_MUTEX(&sys_ep->lock);
    init_completion(&sys_ep->done);
    sys_ep->ep = usb_ep_autoconfig(gadget, bind_desc);
    if (sys_ep->ep) {
      sys_ep->ep->driver_data = gadget_ep_driver_data;
      sys_ep->bound_desc = bind_desc;
      return 0;
    } else {
      return -ENODEV;
    }
  } else {
    return -EINVAL;
  }
}
int sysfs_endpoint_unbind(sysfs_endpoint* sys_ep)
{
  if (!sys_ep) {
    return -EINVAL;
  }
  DBG(sys_ep, "%s\n", __func__);
  if (sys_ep->ep) { sys_ep->ep->driver_data = 0; }
  sys_ep->bound_desc = 0;
  wake_up_interruptible(&sys_ep->waitq);
  return 0;
}

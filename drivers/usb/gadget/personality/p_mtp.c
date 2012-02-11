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

#include "p_mtp.h"
#include "personality.h"
#include "linux/sched.h"
#include "sysfs_endpoint.h"

enum DeviceStatusConstants {
  // We only care for the least significant byte.
  DEVICE_OK               = 0x01,
  DEVICE_BUSY             = 0x19,
  DEVICE_INITIATED_CANCEL = 0x1F
};

struct mtp_function {
  struct kobject kobj;
  struct usb_function function;
  struct usb_composite_dev *cdev;

  struct sysfs_endpoint in, out, inter;

  struct timer_list user_busy_timer;
  wait_queue_head_t waitq;
  atomic_t user_cancel_id;
  atomic_t user_device_status;
  atomic_t user_device_status_used;
  atomic_t user_reset;
  atomic_t user_shutdown;
};
struct mtp_function* kobject_to_mtp_function(struct kobject* kobj)
{
  return kobj ? container_of(kobj, struct mtp_function, kobj) : 0;
}
static struct mtp_function* usb_function_to_mtp_function(struct usb_function* usb_func)
{
  return usb_func ? container_of(usb_func, struct mtp_function, function) : 0;
}
static struct usb_gadget* usb_function_to_gadget(struct usb_function* usb_func)
{
  if (usb_func) {
    struct usb_configuration* config = usb_func->config;
    if (config) {
      struct usb_composite_dev* cdev = config->cdev;
      if (cdev) {
        return cdev->gadget;
      }
    }
  }
  return 0;
}
static void mtp_function_release(struct kobject* kobj)
{
  struct mtp_function* mtp_func = kobject_to_mtp_function(kobj);
  if (!mtp_func) {
    return;
  }
  DBG(mtp_func->cdev, "%s\n", __func__);
  del_timer_sync(&mtp_func->user_busy_timer);
  kfree(mtp_func);
}
static struct kobj_type mtp_function_type = { mtp_function_release, &kobj_sysfs_ops };

static struct usb_interface_descriptor interface_desc = {
  .bLength = USB_DT_INTERFACE_SIZE,
  .bDescriptorType = USB_DT_INTERFACE,
  .bNumEndpoints = 3,
  .bInterfaceClass = 6,
  .bInterfaceSubClass =	1,
  .bInterfaceProtocol =	1,
  .iInterface = USB_PERSONALITY_INTERFACE_MTP
};

static struct usb_endpoint_descriptor fs_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(64),
};
static struct usb_endpoint_descriptor fs_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(64),
};
static struct usb_endpoint_descriptor fs_int_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	__constant_cpu_to_le16(16),
        .bInterval =            __constant_cpu_to_le16(10),
};
static struct usb_descriptor_header *fs_function[] = {
	(struct usb_descriptor_header *) &interface_desc,
	(struct usb_descriptor_header *) &fs_out_desc,
	(struct usb_descriptor_header *) &fs_in_desc,
	(struct usb_descriptor_header *) &fs_int_desc,
	NULL,
};

static struct usb_endpoint_descriptor hs_in_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(512),
};
static struct usb_endpoint_descriptor hs_out_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes =		USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize =	__constant_cpu_to_le16(512),
};
static struct usb_endpoint_descriptor hs_int_desc = {
	.bLength =		USB_DT_ENDPOINT_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize =	__constant_cpu_to_le16(16),
        .bInterval =            __constant_cpu_to_le16(7),  /* 2**(7-1) = 64 uframes ~= 8 ms */
};
static struct usb_descriptor_header *hs_function[] = {
	(struct usb_descriptor_header *) &interface_desc,
	(struct usb_descriptor_header *) &hs_out_desc,
	(struct usb_descriptor_header *) &hs_in_desc,
	(struct usb_descriptor_header *) &hs_int_desc,
	NULL,
};

static ssize_t ep_in_write(struct kobject *kobj, struct bin_attribute *bin_attr,
                           char *buffer, loff_t offset, size_t buffer_size)
{
  struct mtp_function* mtp_func = kobject_to_mtp_function(kobj);
  DBG(mtp_func->cdev, "%s\n", __func__);
  return mtp_func ? sysfs_endpoint_write(&mtp_func->in, buffer, buffer_size) : -EINVAL;
}
static ssize_t ep_out_read(struct kobject *kobj, struct bin_attribute *bin_attr,
                           char *buffer, loff_t offset, size_t buffer_size)
{
  struct mtp_function* mtp_func = kobject_to_mtp_function(kobj);
  DBG(mtp_func->cdev, "%s\n", __func__);
  return mtp_func ? sysfs_endpoint_read(&mtp_func->out, buffer, buffer_size) : -EINVAL;
}
static ssize_t ep_int_write(struct kobject *kobj, struct bin_attribute *bin_attr,
                            char *buffer, loff_t offset, size_t buffer_size)
{
  struct mtp_function* mtp_func = kobject_to_mtp_function(kobj);
  DBG(mtp_func->cdev, "%s\n", __func__);
  return mtp_func ? sysfs_endpoint_write(&mtp_func->inter, buffer, buffer_size) : -EINVAL;
}

static ssize_t ep_int_stall(struct kobject *kobj, struct bin_attribute *bin_attr,
                            char *buffer, loff_t offset, size_t buffer_size)
{
  struct mtp_function* mtp_func = kobject_to_mtp_function(kobj);
  return mtp_func ? sysfs_endpoint_stall(&mtp_func->inter) : -EINVAL;
}

static bool is_user_control_pending(struct mtp_function* mtp_func)
{
  return (-1 != atomic_read(&mtp_func->user_cancel_id)
          || 0 != atomic_read(&mtp_func->user_device_status_used)
          || atomic_read(&mtp_func->user_reset)
          || atomic_read(&mtp_func->user_shutdown));
}

static ssize_t control_read(struct kobject *kobj, struct bin_attribute *bin_attr,
                            char *buffer, loff_t offset, size_t buffer_size)
{
  struct mtp_function* mtp_func = kobject_to_mtp_function(kobj);
  DBG(mtp_func->cdev, "%s\n", __func__);

  if (!mtp_func) {
    return -EINVAL;
  }
  while (true) { /* Loop until something happened */
    if (wait_event_interruptible(mtp_func->waitq, is_user_control_pending(mtp_func))) {
      return -ERESTARTSYS;
    } else if (atomic_read(&mtp_func->user_shutdown)) {
      return -ESHUTDOWN;
    } else if (atomic_read(&mtp_func->user_reset)) {
      atomic_set(&mtp_func->user_reset, 0);
      return 0;
    } else if (atomic_read(&mtp_func->user_device_status_used)) {
      atomic_set(&mtp_func->user_device_status_used, 0);
      return 1;
    } else {
      int cancel_id = atomic_read(&mtp_func->user_cancel_id);
      if (-1 != cancel_id) {
        size_t size = min(sizeof(cancel_id), buffer_size);
        memcpy(buffer, &cancel_id, size);
        atomic_set(&mtp_func->user_cancel_id, -1);
        return size;
      }
    }
  }
}

static ssize_t control_write(struct kobject *kobj, struct bin_attribute *bin_attr,
                             char *buffer, loff_t offset, size_t buffer_size)
{
  struct mtp_function* mtp_func = kobject_to_mtp_function(kobj);
  DBG(mtp_func->cdev, "%s\n", __func__);

  if (!mtp_func) {
    return -EINVAL;
  }
  if (1 >= buffer_size) {
    sysfs_endpoint_stall(&mtp_func->out);
    sysfs_endpoint_stall(&mtp_func->in);
  } else {
    atomic_set(&mtp_func->user_device_status, buffer[buffer_size-1]);
    atomic_set(&mtp_func->user_device_status_used, 0);
    mod_timer(&mtp_func->user_busy_timer, jiffies + 5 * HZ); /* Go back to "busy" in 5 seconds */
  }
  return buffer_size;
}

static struct bin_attribute ep_out_attr = {
  .attr = { .name = "out", .mode = 0666, },
  .read = ep_out_read,
  .write = control_write
};
static struct bin_attribute ep_in_attr  = {
  .attr = { .name = "in", .mode = 0666, },
  .read = control_read,
  .write = ep_in_write
};
static struct bin_attribute ep_int_attr = {
  .attr = { .name = "int", .mode = 0666, },
  .read = ep_int_stall,
  .write = ep_int_write
};

int send_mtp_extended_compat_id(struct usb_configuration* usb_config,
                                const struct usb_ctrlrequest* ctrl_req)
{
  int result = -EOPNOTSUPP;
  struct usb_composite_dev* cdev = usb_config->cdev;
  if (USB_PERSONALITY_GET_MICROSOFT_OS_FEATURE_DESCRIPTOR_REQUEST == ctrl_req->bRequest) {
    if ((USB_DIR_IN|USB_TYPE_VENDOR|USB_RECIP_DEVICE) == ctrl_req->bRequestType) {
      static const unsigned char ms_os_feature_descriptor[] = {
        0x28, 0x00, 0x00, 0x00, // Length = 40.
        0x00, 0x01, // Release = 1.0.
        0x04, 0x00, // Index 0x04 for extended compat ID descriptors.
        0x01, // Number of following function sections.
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Reserved
        0x00, // Starting Interface number.
        0x01, // Reserved := 1
        'M', 'T', 'P', 0x00, 0x00, 0x00, 0x00, 0x00, // Compatible ID for MTP.
        '4', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Sub compatible ID.
        // 0=none, '1'=generic, '4'=mobile
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // Reserved
      };
      int length = min(le16_to_cpu(ctrl_req->wLength), (u16) sizeof ms_os_feature_descriptor);
      memcpy(cdev->req->buf, ms_os_feature_descriptor, length);

      cdev->req->zero = 0;
      cdev->req->length = length;
      result = usb_ep_queue(cdev->gadget->ep0, cdev->req, GFP_ATOMIC);
      if (result) {
        WARNING(cdev, "failed sending extended compat id, result 0x%x\n", result);
      }
    }
  }
  return result;
}

int send_mtp_none_extended_compat_id(struct usb_configuration* usb_config,
                                     const struct usb_ctrlrequest* ctrl_req)
{
  int result = -EOPNOTSUPP;
  struct usb_composite_dev* cdev = usb_config->cdev;
  if (USB_PERSONALITY_GET_MICROSOFT_OS_FEATURE_DESCRIPTOR_REQUEST == ctrl_req->bRequest) {
    if ((USB_DIR_IN|USB_TYPE_VENDOR) == ctrl_req->bRequestType) {
      static const unsigned char ms_os_feature_descriptor[] = {
        0x40, 0x00, 0x00, 0x00, // Length = 64.
        0x00, 0x01, // Release = 1.0.
        0x04, 0x00, // Index 0x04 for extended compat ID descriptors.
        0x02, // Number of following function sections.
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Reserved
        0x00, // Starting Interface number.
        0x01, // Reserved := 1
        'M', 'T', 'P', 0x00, 0x00, 0x00, 0x00, 0x00, // Compatible ID for MTP.
        '4', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Sub compatible ID.
        // 0=none, '1'=generic, '4'=mobile
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Reserved
        0x01, // Starting Interface number.
        0x01, // Reserved := 1
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // No compatible ID.
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // No subcompatible ID.
        // 0=none, '1'=generic, '4'=mobile
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // Reserved
      };
      int length = min(le16_to_cpu(ctrl_req->wLength), (u16) sizeof ms_os_feature_descriptor);
      memcpy(cdev->req->buf, ms_os_feature_descriptor, length);

      cdev->req->zero = length && ((length % 64) == 0);
      cdev->req->length = length;
      result = usb_ep_queue(cdev->gadget->ep0, cdev->req, GFP_ATOMIC);
      if (result) {
        WARNING(cdev, "failed sending extended compat id, result 0x%x\n", result);
      }
    }
  }
  return result;
}

static int setup_status(struct usb_composite_dev* cdev, struct usb_request* req, u16 len)
{
  struct mtp_function* mtp_func = (struct mtp_function*)req->context;
  unsigned char device_status[12] = { 4, 0, DEVICE_BUSY, 0x20 };
  DBG(mtp_func->cdev, "%s\n", __func__);

  device_status[2] = atomic_read(&mtp_func->user_device_status);
  if (DEVICE_INITIATED_CANCEL == device_status[2]) {
    struct usb_endpoint_descriptor* out_desc = mtp_func->out.enabled_desc;
    struct usb_endpoint_descriptor* in_desc = mtp_func->in.enabled_desc;
    device_status[0] = 12; /* Increase length and set end */
    if (out_desc) {
      device_status[4] = out_desc->bEndpointAddress;
    }
    if (in_desc) {
      device_status[8] = in_desc->bEndpointAddress;
    }
  }
  atomic_set(&mtp_func->user_device_status_used, 1);
  wake_up_interruptible(&mtp_func->waitq);

  req->length = min(len, (u16)device_status[0]);
  req->zero = (0 == req->length);
  memcpy(req->buf, device_status, req->length);
  {
    int * idata = (int*) req->buf;
    DBG(mtp_func->cdev, "%s: %08x %08x %08x\n", __func__, idata[0], idata[1], idata[2]);
  }
  return 0;
}
static void setup_cancel_complete(struct usb_ep *ep, struct usb_request *req)
{
  struct mtp_function* mtp_func = (struct mtp_function*)req->context;
  unsigned char * buf = req->buf;
  if (0 == req->status && 6 == req->actual && 0x01 == buf[0] && 0x40 == buf[1]) {
    int id = buf[2] + (buf[3]<<8) + (buf[4] << 16) + (buf[5] << 24);
    atomic_set(&mtp_func->user_cancel_id, id);
    wake_up_interruptible(&mtp_func->waitq);
    DBG(mtp_func->cdev, "%s: cancel(0x%x)\n", __func__, id);
  } else {
    WARNING(mtp_func->cdev, "%s failed, status:%d actual:%d\n", __func__, req->status, req->actual);
  }
}
static int setup_cancel(struct usb_composite_dev* cdev, struct usb_request *req, u16 len)
{
  struct mtp_function* mtp_func = (struct mtp_function*)req->context;
  DBG(mtp_func->cdev, "%s len:%d\n", __func__, len);
  if (6 == len) {
    req->complete = setup_cancel_complete;
    atomic_set(&mtp_func->user_device_status_used, 1);
    atomic_set(&mtp_func->user_device_status, DEVICE_BUSY);
    wake_up_interruptible(&mtp_func->waitq);
    return 0;
  } else {
    return -EINVAL;
  }
}
static int setup_reset(struct usb_composite_dev* cdev, struct usb_request *req, u16 len)
{
  struct mtp_function * mtp_func = req->context;
  DBG(mtp_func->cdev, "%s len:%d\n", __func__, len);
  if (0 == len) {
    atomic_set(&mtp_func->user_reset, 1);
    atomic_set(&mtp_func->user_device_status_used, 1);
    atomic_set(&mtp_func->user_device_status, DEVICE_BUSY);
    wake_up_interruptible(&mtp_func->waitq);
    DBG(mtp_func->cdev, "%s: event(RESET)\n", __func__);
    req->length = 0;
    return 0;
  } else {
    return -EINVAL;
  }
}

static void mtp_unbind(struct usb_configuration* usb_conf, struct usb_function* usb_func)
{
  struct mtp_function* mtp_func = usb_function_to_mtp_function(usb_func);
  if (mtp_func) {
    DBG(mtp_func->cdev, "mtp_unbind\n");
    atomic_set(&mtp_func->user_shutdown, 1); /* Unblock control reader. */
    wake_up_interruptible(&mtp_func->waitq);
    sysfs_endpoint_unbind(&mtp_func->in);
    sysfs_endpoint_unbind(&mtp_func->out);
    sysfs_endpoint_unbind(&mtp_func->inter);
    kobject_put(&mtp_func->kobj);
  }
}
static int  ptp_bind(struct usb_configuration* usb_conf, struct usb_function* usb_func)
{
  struct mtp_function* mtp_func = usb_function_to_mtp_function(usb_func);
  struct usb_gadget* gadget = usb_function_to_gadget(usb_func);

  if (mtp_func && gadget) {
    struct kobject* kobj = &mtp_func->kobj;
    int error = 0;
    DBG(mtp_func->cdev, "%s\n", __func__);
    kobject_get(kobj);
    sysfs_endpoint_bind(&mtp_func->in,    &fs_in_desc,  gadget, mtp_func);
    sysfs_endpoint_bind(&mtp_func->out,   &fs_out_desc, gadget, mtp_func);
    sysfs_endpoint_bind(&mtp_func->inter, &fs_int_desc, gadget, mtp_func);
    if (!mtp_func->in.ep || !mtp_func->out.ep || !mtp_func->inter.ep) {
      mtp_unbind(usb_conf, usb_func);
      return -ENODEV;
    }

    DBG(mtp_func->cdev, "%s: will create bin file (%p, %p)\n", __func__, kobj, &ep_out_attr);
    if (!error) { error = sysfs_create_bin_file(kobj, &ep_out_attr); }
    DBG(mtp_func->cdev, "%s: did create bin file (0x%x)\n", __func__, error);
    if (!error) { error = sysfs_create_bin_file(kobj, &ep_in_attr); }
    DBG(mtp_func->cdev, "%s: did create bin file (0x%x)\n", __func__, error);
    if (!error) { error = sysfs_create_bin_file(kobj, &ep_int_attr); }
    DBG(mtp_func->cdev, "%s: did create bin file (0x%x)\n", __func__, error);
    if (error) {
      mtp_unbind(usb_conf, usb_func);
      return -EFAULT;
    }

    usb_func->descriptors = /* usb_copy_descriptors */ fs_function;
    if (gadget_is_dualspeed(gadget)) {
      hs_in_desc.bEndpointAddress = fs_in_desc.bEndpointAddress;
      hs_out_desc.bEndpointAddress = fs_out_desc.bEndpointAddress;
      hs_int_desc.bEndpointAddress = fs_int_desc.bEndpointAddress;
      usb_func->hs_descriptors = /* usb_copy_descriptors */ hs_function;
    }
    error = kobject_uevent(kobj, KOBJ_ADD);
    if (error) { /* log a warning, but return success */
      WARNING(mtp_func->cdev, "failed sending uevent (%d)\n", error);
    }

    return 0;
  } else {
    return -EINVAL;
  }
}
/* LGE_CHANGE_S kenneth.kang@lge.com 2011-01-16 USB patch */
static int  mtp_bind(struct usb_configuration* usb_conf, struct usb_function* usb_func)
{
  interface_desc.iInterface = USB_PERSONALITY_INTERFACE_MTP;
  return ptp_bind(usb_conf, usb_func);
}
static int  dps_bind(struct usb_configuration* usb_conf, struct usb_function* usb_func)
{
  interface_desc.iInterface = USB_PERSONALITY_INTERFACE_DPS;
  return ptp_bind(usb_conf, usb_func);
}
/* LGE_CHANGE_E kenneth.kang@lge.com 2011-01-16 USB patch */
static int  mtp_set_alt(struct usb_function* usb_func, unsigned interface, unsigned alt)
{
  struct mtp_function* mtp_func = usb_function_to_mtp_function(usb_func);
  struct usb_gadget *gadget = usb_function_to_gadget(usb_func);
  int error = 0;

  if (!mtp_func || !gadget) { return -EINVAL; }
  DBG(mtp_func->cdev, "mtp_set_alt\n");
  error = sysfs_endpoint_enable(&mtp_func->in, ep_choose(gadget, &hs_in_desc, &fs_in_desc));
  if (error) { return error; }
  error = sysfs_endpoint_enable(&mtp_func->out, ep_choose(gadget, &hs_out_desc, &fs_out_desc));
  if (error) { return error; }
  error = sysfs_endpoint_enable(&mtp_func->inter, ep_choose(gadget, &hs_int_desc, &fs_int_desc));
  return error;
}
/* static int  mtp_get_alt(struct usb_function* f, unsigned interface) */
static void mtp_disable(struct usb_function* usb_func)
{
  struct mtp_function* mtp_func = usb_function_to_mtp_function(usb_func);
  /* struct usb_gadget *gadget = usb_function_to_gadget(usb_func); */

  DBG(mtp_func->cdev, "mtp_disable\n");
  if (!mtp_func /* || !gadget */) { return; }

  sysfs_endpoint_disable(&mtp_func->in);
  sysfs_endpoint_disable(&mtp_func->out);
  sysfs_endpoint_disable(&mtp_func->inter);
}
static int  mtp_setup(struct usb_function* usb_func, const struct usb_ctrlrequest *ctrl_req)
{
  enum { CANCEL = 0x64, RESET = 0x66, GET_DEVICE_STATUS = 0x67 };
  struct usb_composite_dev* cdev = usb_func->config->cdev;
  struct usb_request *req = cdev->req;
  u16 len = le16_to_cpu(ctrl_req->wLength);
  int result;
  DBG(cdev, "%s: req:0x%x type:0x%x\n", __func__, ctrl_req->bRequest, ctrl_req->bRequestType);
  req->context = usb_function_to_mtp_function(usb_func);

  switch (ctrl_req->bRequest | (ctrl_req->bRequestType << 8)) {
  case GET_DEVICE_STATUS | ((USB_DIR_IN|USB_TYPE_CLASS|USB_RECIP_INTERFACE) << 8):
    result = setup_status(cdev, req, len);
    break;
  case CANCEL | ((USB_DIR_OUT|USB_TYPE_CLASS|USB_RECIP_INTERFACE) << 8):
    result = setup_cancel(cdev, req, len);
    break;
  case RESET | ((USB_DIR_OUT|USB_TYPE_CLASS|USB_RECIP_INTERFACE) << 8):
    result = setup_reset(cdev, req, len);
    break;
  default:
    result = -EOPNOTSUPP;
    break;
  }
  if (result >= 0) {
    result = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
  }
  return result;
}
/* static void mtp_suspend(struct usb_function* f) {}
   static void mtp_resume(struct usb_function* f) {} */

static void user_busy_watchdog(unsigned long user_device_status_pointer)
{
  atomic_set((atomic_t*)user_device_status_pointer, DEVICE_BUSY);
}
/* LGE_CHANGE_S kenneth.kang@lge.com 2011-01-16 USB patch */
typedef int (*my_bind_function)(struct usb_configuration*, struct usb_function*);
static int ptp_function_add(struct usb_composite_dev *cdev, struct usb_configuration *usb_conf,
                            const char* protocol, my_bind_function my_bind)
{
  int interface_number = -1;
  struct mtp_function* mtp_func = 0;
  struct usb_function* usb_func = 0;
  struct kobject* kobj = 0;
  int error = 0;

  if (!cdev || !cdev->gadget || !usb_conf) { return -EINVAL; }
  DBG(cdev, "%s\n", __func__);

  mtp_func = kzalloc(sizeof(*mtp_func), GFP_KERNEL);
  if (!mtp_func) {
    return -ENOMEM;
  }
/* LGE_CHANGE_E kenneth.kang@lge.com 2011-01-16 USB patch */
  init_waitqueue_head(&mtp_func->waitq);
  atomic_set(&mtp_func->user_cancel_id, /* INVALID_TRANSACTION_ID */ -1);
  atomic_set(&mtp_func->user_device_status, /* DEVICE_BUSY & 0xFF */ 0x19);
  atomic_set(&mtp_func->user_device_status_used, /* no status update pending */ 0);
  atomic_set(&mtp_func->user_reset, /* no reset pending */ 0);
  atomic_set(&mtp_func->user_shutdown, /* not shutting down */ 0);
  init_timer(&mtp_func->user_busy_timer);
  mtp_func->user_busy_timer.function = user_busy_watchdog;
  mtp_func->user_busy_timer.data = (unsigned long) &mtp_func->user_device_status;

  mtp_func->cdev = cdev;
  usb_func = &mtp_func->function;
  kobj = &mtp_func->kobj;

  interface_number = usb_interface_id(usb_conf, usb_func);
  if (interface_number < 0) {
    kfree(mtp_func);
    return /* error = */ interface_number;
  }
  interface_desc.bInterfaceNumber = interface_number;

  usb_func->name = protocol;
  usb_func->bind = my_bind;
  usb_func->unbind = mtp_unbind;
  usb_func->setup = mtp_setup;
  usb_func->set_alt = mtp_set_alt;
  usb_func->disable = mtp_disable;

  kobj->kset = personality_cdev_to_kset(cdev);
  error = kobject_init_and_add(kobj, &mtp_function_type, &cdev->gadget->dev.kobj, protocol);
  if (!error) {
    error = usb_add_function(usb_conf, usb_func);
  }
  kobject_put(kobj);
  return error;
}
/* LGE_CHANGE_S kenneth.kang@lge.com 2011-01-16 USB patch */
int dps_function_add(struct usb_composite_dev *cdev, struct usb_configuration *usb_conf)
{
  return ptp_function_add(cdev, usb_conf, "dps", dps_bind);
}
int mtp_function_add(struct usb_composite_dev *cdev, struct usb_configuration *usb_conf)
{
  return ptp_function_add(cdev, usb_conf, "mtp", mtp_bind);
}
/* LGE_CHANGE_E kenneth.kang@lge.com 2011-01-16 USB patch */
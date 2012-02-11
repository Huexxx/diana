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
#include "p_composite.c"
#include "linux/sched.h"
#include "linux/semaphore.h"
#include "p_communications.h"
#include "p_lge_vtp.h"
#include "p_lge_normal.h"
#include "p_mtp_dps_adb.h"
#include "personality.h"
#include "p_lge_ums.h"
#include <linux/string.h> // jjun.lee

// jjun.lee
#define MAX_USB_SERIAL_NUM		33
extern char device_serial_for_usb[MAX_USB_SERIAL_NUM]; 
// jjun.lee

struct factory {
  int value;
  const char* name;
  void (*add_configurations)(struct usb_composite_dev* cdev, const char* name);
};
static struct factory personalities[] = {
/* LGE_CHANGE_S kenneth.kang@lge.com 2011-01-16 USB patch */
  { 1, "PC suite with ADB", add_lge_normal_adb_configurations },
  { 2, "PC suite without ADB", add_lge_normal_configurations },
  { USB_CDROM_FOR_PCSUITE, "CDROM for PC suite", add_lge_vtp_configurations },
  { USB_CDROM_FOR_CP, "CDROM for CP", add_lge_vtp_configurations },
  { 4, "CP", switch_to_communications_processor },
  { 5, "MTP", add_mtp_configurations },
  { 6, "MTP & ADB", add_mtp_adb_configurations },
  { 7, "PictBridge", add_dps_configurations },
  { 8, "UMS", add_lge_ums_configurations },
/* LGE_CHANGE_E kenneth.kang@lge.com 2011-01-16 USB patch */
  {} };
static bool is_personality_supported(int tested_value)
{
  struct factory * iter = 0;
  for (iter = personalities; iter->value && iter->name; ++iter) {
    if (iter->value == tested_value) return true;
  }
  return false;
}

struct personality {
  struct usb_composite_driver driver;
  struct workqueue_struct* restart_worker;
  struct work_struct restart;
  struct usb_gadget *gadget;
  struct sysfs_dirent *state_sd;
  bool attached; /**< The value is based on the last usb_gadget_vbus_connect/disconnect() call. */
  bool configured; /**< True after a setup() call, false initially and after a disconnect() call */
  int personality; /**< The current personality or zero if no selection was made */
  int next_personality; /**< The next personality when the gadget is being reconfigured */
  struct rw_semaphore next_personality_lock;
  struct kset *kset;
  /** The gadget's original setup() function pointer is used when overriding */
  int (*setup_super)(struct usb_gadget*, const struct usb_ctrlrequest*);
  /** The gadget's original disconnect() function pointer is used when overriding */
  void (*disconnect_super)(struct usb_gadget*);
  int (*vbus_session_super) (struct usb_gadget *, int is_active);
  struct usb_gadget_ops gadget_ops_sub;
  const struct usb_gadget_ops *gadget_ops_super;
};
static struct personality* cdev_to_personality(struct usb_composite_dev* cdev)
{
  if (!cdev || !cdev->driver) {
    return 0;
  }
  return container_of(cdev->driver, struct personality, driver);
}

static ssize_t get_state(struct device* dev, struct device_attribute* att, char* buf)
{
  struct usb_composite_dev* cdev = dev ? dev_get_drvdata(dev) : 0;
  struct personality* pers = cdev_to_personality(cdev);
  dev_dbg(dev, "get_state\n");
  if (pers && buf && down_read_trylock(&pers->next_personality_lock)) {
    int pos = scnprintf(buf, PAGE_SIZE, "{ \"personality\" : %d", pers->personality);
    if (pers->attached) {
      pos += scnprintf(buf+pos, PAGE_SIZE-pos, ", \"attached\" : true");
      if (pers->configured) {
        pos += scnprintf(buf+pos, PAGE_SIZE-pos, ", \"configured\" : true");
        if (USB_SPEED_HIGH == cdev->gadget->speed) {
          pos += scnprintf(buf+pos, PAGE_SIZE-pos, ", \"high-speed\" : true");
        } else if (USB_SPEED_FULL == cdev->gadget->speed) {
          pos += scnprintf(buf+pos, PAGE_SIZE-pos, ", \"full-speed\" : true");
        } else if (USB_SPEED_LOW == cdev->gadget->speed) {
          pos += scnprintf(buf+pos, PAGE_SIZE-pos, ", \"low-speed\" : true");
        }
      }
    }
    pos += scnprintf(buf+pos, PAGE_SIZE-pos, " }\n");
    up_read(&pers->next_personality_lock);
    dev_dbg(dev, "get_state: %s", buf);
    return pos; /* fs/sysfs/file.c fill_read_buffer() checks result range */
  } else {
    dev_dbg(dev, "get_state: ECONNRESET\n");
    return -ECONNRESET;
  }
}
int get_personality(struct usb_composite_dev* cdev)
{
  struct personality* pers = cdev_to_personality(cdev);
  return pers->personality;
}
int set_next_personality(struct usb_composite_dev* cdev, int next)
{
  struct personality* pers = cdev_to_personality(cdev);
  if (0 != next && !is_personality_supported(next)) {
    return -ENOTSUPP;
  }
  if (pers->personality != next) {
    pers->next_personality = next;
    queue_work(pers->restart_worker, &pers->restart);
  }
  return 0;
}
static ssize_t set_state(struct device* dev, struct device_attribute* att, const char* buf,
                         size_t size)
{
  struct usb_composite_dev* cdev = dev ? dev_get_drvdata(dev) : 0;
  int next = 0;
  int scanned_fields = 0;
  int result;
  dev_dbg(dev, "set_state: %s", buf);
  if (!size || buf[size-1] != '\n') {
    return -EINVAL;
  }
  scanned_fields = sscanf(buf, "{ \"personality\" : %d }", &next);
  if (scanned_fields != 1) {
    return -EFAULT;
  }
  if (0 != next && !is_personality_supported(next)) {
    return -ENOTSUPP;
  }
  result = set_next_personality(cdev, next);
  return result < 0 ? result : size;
}
static ssize_t get_personalities(struct device* dev, struct device_attribute* attr, char* buf)
{
  int pos = 0;
  struct factory * iter = 0;
  for (iter = personalities; iter->value && iter->name; ++iter)
  {
    char delim = pos ? ',' : '{';
    pos += scnprintf(buf+pos, PAGE_SIZE-pos, "%c \"%s\" : %d", delim, iter->name, iter->value);
  }
  pos = pos + scnprintf(buf+pos, PAGE_SIZE-pos, "%c}\n", pos ? ' ' : '{');
  return pos; /* fs/sysfs/file.c fill_read_buffer() checks result range */
}

static struct device_attribute state_attribute = {
  { "state", .mode = 0666 },
  &get_state, &set_state
};
static struct device_attribute personalities_attribute = {
  { "personalities", .mode = 0666 },
  &get_personalities
};

static int vbus_session_sub(struct usb_gadget* gadget, int is_active)
{
  struct personality* pers = cdev_to_personality(get_gadget_data(gadget));
  DBG(pers, "vbus_session_sub ----++----\n");
  if (pers) {
    const struct usb_gadget_ops *ops = pers->gadget_ops_super;
    const bool new_attached = is_active;
    DBG(pers, "vbus_session_sub(%p, %d)\n", gadget, is_active);
    if (new_attached != pers->attached) {
      struct sysfs_dirent *state_sd = pers->state_sd;
      pers->attached = new_attached;
      if (state_sd) { sysfs_notify_dirent(state_sd); }
      else {DBG(pers, "vbus_session_sub - state_sd: NULL ----++----\n");}
    }
    if (ops) {
      if (ops->vbus_session) {
        return (*ops->vbus_session)(gadget, is_active);
      } else {
        return -EOPNOTSUPP;
      }
    }
  }
  return -EFAULT;
}
static int setup_sub(struct usb_gadget* gadget, const struct usb_ctrlrequest* request)
{
  struct personality* pers = cdev_to_personality(get_gadget_data(gadget));
  DBG(pers, "setup_sub ----++----\n");
  if (pers && pers->setup_super) {
    if (!pers->configured) {
/* LGE_CHANGE_S kenneth.kang@lge.com 2011-01-16 USB patch */	
     struct sysfs_dirent *state_sd = pers->state_sd;
      pers->attached = true;
      pers->configured = true;
      if (state_sd) { sysfs_notify_dirent(state_sd); }
      else {DBG(pers, "setup_sub - state_sd: NULL ----++----\n");}
/* LGE_CHANGE_E kenneth.kang@lge.com 2011-01-16 USB patch */	  
    }
    return (*pers->setup_super)(gadget, request);

    /* if (pers->personality) { */
    /*   return (*pers->setup_super)(gadget, request); */
    /* } else { */
    /*   /\* Turn pullup off.  This should trigger a disconnect_sub call, */
    /*      which will inform observers about the changed state. *\/ */
    /*   usb_gadget_disconnect(gadget);  */
    /*   return 0; */
    /* } */
  } else {
    return -EFAULT;
  }
}
static void disconnect_sub(struct usb_gadget* gadget)
{
  struct personality* pers = cdev_to_personality(get_gadget_data(gadget));
/* LGE_CHANGE_S kenneth.kang@lge.com 2011-01-16 USB patch */  
  DBG(pers, "disconnect_sub ----++----\n");
  DBG(pers, "disconnect_sub\n");
  if (pers) {
    if (pers->configured) {
      struct sysfs_dirent *state_sd = pers->state_sd;
      DBG(pers, "disconnect_sub will notfiy unconfigured\n");
      pers->configured = false;
      if (state_sd) { sysfs_notify_dirent(state_sd); }
      else {DBG(pers, "disconnect_sub - state_sd: NULL ----++----\n");}	  
    }
    if (pers->disconnect_super) {
    (*pers->disconnect_super)(gadget);
    }
/* LGE_CHANGE_E kenneth.kang@lge.com 2011-01-16 USB patch */	
  }
}

static void restart_with_next_personality(struct work_struct *w)
{
  int error = 0;
  struct personality *pers = container_of(w, struct personality, restart);
  struct usb_device_descriptor *dev = (struct usb_device_descriptor*) pers->driver.dev;
  DBG(pers, "restart_with_next_personality\n");
  down_write(&pers->next_personality_lock);
  usb_composite_unregister(&pers->driver);
  DBG(pers, "restart_with_next_personality: unregister returned\n");
  pers->personality = pers->next_personality;
  dev->idProduct = pers->personality;
  error = usb_gadget_disconnect(pers->gadget); /* pullup(0) */
  if (error) { WARNING(pers, "restart_with_next_personality: disconnect failed\n"); }
  /* msleep(10); */
  error = usb_composite_register(&pers->driver);
  DBG(pers, "restart_with_next_personality: register returned\n");
  if (error) {
    printk(KERN_WARNING "restart_with_next_personality: composite_register failed\n");
  }
  if (pers->personality) {
    error = usb_gadget_connect(pers->gadget); /* pullup(1) */
    DBG(pers, "restart_with_next_personality: usb_gadget_connect returned\n");	
  } else {
    error = usb_gadget_disconnect(pers->gadget);  /* No personality selected: pullup(0) */
    DBG(pers, "restart_with_next_personality: usb_gadget_disconnect returned\n");	
  }
  if (error) { WARNING(pers, "restart_with_next_personality: ..connect failed\n"); }
  up_write(&pers->next_personality_lock);
}
static void override_vbus_setup_and_disconnect(struct usb_composite_dev* cdev)
{
  struct personality* pers = container_of(cdev->driver, struct personality, driver);
  struct device_driver* ddriver = cdev->gadget->dev.driver;
  struct usb_gadget_driver* gdriver = container_of(ddriver, struct usb_gadget_driver, driver);
  pers->setup_super = gdriver->setup;
  gdriver->setup = setup_sub;
  pers->disconnect_super = gdriver->disconnect;
  gdriver->disconnect = disconnect_sub;
  /* We don't edit ops since it's const.  Editing a copy seems safer. */
  pers->gadget_ops_super = cdev->gadget->ops;
  memcpy(&pers->gadget_ops_sub, cdev->gadget->ops, sizeof(pers->gadget_ops_sub));
  pers->gadget_ops_sub.vbus_session = vbus_session_sub;
  cdev->gadget->ops = &pers->gadget_ops_sub;
}
static void restore_vbus_setup_and_disconnect(struct usb_composite_dev* cdev)
{
  struct personality* pers = container_of(cdev->driver, struct personality, driver);
  struct device_driver* ddriver = cdev->gadget->dev.driver;
  struct usb_gadget_driver* gdriver = container_of(ddriver, struct usb_gadget_driver, driver);
  DBG(pers, "%s\n", __func__);
  gdriver->setup = pers->setup_super;
  pers->setup_super = NULL;
  gdriver->disconnect = pers->disconnect_super;
  pers->disconnect_super = NULL;
  cdev->gadget->ops = pers->gadget_ops_super;
  pers->gadget_ops_super = NULL;
}
static void create_additional_driver_attributes(struct device* dev) {
  int error = 0;
  dev_dbg(dev, "create_additional_driver_attributes\n");
  error = device_create_file(dev, &state_attribute);
  if (!error) {
    error = device_create_file(dev, &personalities_attribute);
  } else {
    dev_warn(dev, "device_create_file failed (0x%x)", error);
  }
}
static void remove_additional_driver_attributes(struct device* dev) {
  dev_dbg(dev, "remove_additional_driver_attributes\n");
  device_remove_file(dev, &personalities_attribute);
  dev_dbg(dev, "remove_additional_driver_attributes: did remove 'personalities'\n");
  device_remove_file(dev, &state_attribute);
  dev_dbg(dev, "remove_additional_driver_attributes: did remove 'state'\n");
}
static void add_personality_configurations(struct usb_composite_dev* cdev) {
  struct personality* pers = container_of(cdev->driver, struct personality, driver);
  struct factory* fact = personalities;
  DBG(cdev, "add_personality_configurations\n");
  while (fact->value && fact->value != pers->personality) {
    ++fact;
  }
  if (fact->value && fact->add_configurations) {
    fact->add_configurations(cdev, fact->name);
  }
}

static int personality_bind(struct usb_composite_dev* cdev)
{
  if (cdev && cdev->gadget && cdev->driver) {
    struct personality* pers = cdev_to_personality(cdev);
    DBG(cdev, "personality_bind\n");
    pers->gadget = cdev->gadget;
    pers->attached = false;
    pers->configured = false;
    override_vbus_setup_and_disconnect(cdev);
    create_additional_driver_attributes(&cdev->gadget->dev);
    pers->state_sd = sysfs_get_dirent(cdev->gadget->dev.kobj.sd, state_attribute.attr.name);
    DBG(cdev, "personality_bind: did get sysfs dirent\n");
    add_personality_configurations(cdev);
    return 0;
  } else {
    return -EINVAL;
  }
}
static int personality_unbind(struct usb_composite_dev* cdev)
{
  if (cdev && cdev->gadget) {
    DBG(cdev, "personality_unbind\n");
    if (cdev_to_personality(cdev)->state_sd) {
      /* Poll doesn't return with an error when the sysfs attribute
         disappears.  Until this is examined and fixed we call
         sysfs_notify_dirent() and try to destroy everything before the user can
         reopen the attribute. */
      DBG(cdev, "personality_unbind: notify\n");
      sysfs_notify_dirent(cdev_to_personality(cdev)->state_sd);
      DBG(cdev, "personality_unbind: will put sysfs dirent\n");
      sysfs_put(cdev_to_personality(cdev)->state_sd);
      cdev_to_personality(cdev)->state_sd = NULL;
      DBG(cdev, "personality_unbind: did put sysfs dirent\n");
    }
    remove_additional_driver_attributes(&cdev->gadget->dev);
    if (cdev->driver) {
      restore_vbus_setup_and_disconnect(cdev);
    }
  }
  return 0;
}
static struct personality* instance(void)
{
  static struct usb_string strings[] = {
    { .id = USB_PERSONALITY_MANUFACTURER,         .s = "LGE" },
    { .id = USB_PERSONALITY_PRODUCT,              .s = "LG-P970"},
    { .id = USB_PERSONALITY_SERIAL_NUMBER,        .s = "0123456789ABCDEF0123456789ABCDEF"},
    { .id = USB_PERSONALITY_OS_STRING_DESCRIPTOR, .s = "MSFT100\xc3\xbe" },
    { .id = USB_PERSONALITY_INTERFACE_ADB,        .s = "Android Debug Bridge" },
    { .id = USB_PERSONALITY_INTERFACE_CDC,        .s = "Communications Device" },
    { .id = USB_PERSONALITY_INTERFACE_DPS,        .s = "PictBridge" },
    { .id = USB_PERSONALITY_INTERFACE_MTP,        .s = "Media Transfer" },
    {}
  };
  static struct usb_gadget_strings strings_en_US = { .language = 0x0409, .strings = strings };
  static struct usb_gadget_strings strings_zero  = { .language = 0, .strings = strings };
  static struct usb_gadget_strings *gadget_strings[] = { &strings_en_US, &strings_zero, 0 };
  static struct usb_device_descriptor device_desc = {
    .bLength              = sizeof(device_desc),
    .bDescriptorType      = USB_DT_DEVICE,
    .bcdUSB               = __constant_cpu_to_le16(0x0200),
    .idVendor             = __constant_cpu_to_le16(0x1004),
    .idProduct            = __constant_cpu_to_le16(0x0000),
    .bcdDevice            = __constant_cpu_to_le16(0x0100),
    .iManufacturer        = USB_PERSONALITY_MANUFACTURER,
    .iProduct             = USB_PERSONALITY_PRODUCT,
    .iSerialNumber        = USB_PERSONALITY_SERIAL_NUMBER,
    .bNumConfigurations   = 1
  };
  static struct personality result = {
    .driver = { .name    = "personality",
                .dev     = &device_desc,
                .strings = gadget_strings,
                .bind    = personality_bind,
                .unbind  = personality_unbind },

    /* NOTE: When built-in, the default personality is set at the boot-time, before init is started.
     * So, when the personality rely on any init action on the userspace it will not be performed!
     * Solution: Leave the default personality unset here (zero = usb disabled) and set it from init,
     * after init startet (e.g. from init.lge.rc), then also other init rules will be performed and
     * the user space deamons will be started (or images will be mounted) as stated in other init
     * rules. */

    /* .personality = 0x0001, /\* default personality *\/ */
  };
  static bool initialized = false;

  strcpy(strings[2].s, device_serial_for_usb); // jjun.lee
  
  if (!initialized) {
    init_rwsem(&result.next_personality_lock);
    initialized = true;
  }
  return &result;
}

static int __init init(void)
{
  struct personality* pers = instance();
  int error;
  printk(KERN_INFO "personality init\n");
  pers->restart_worker = create_singlethread_workqueue("gpers-work");
  if (!pers->restart_worker) {
    return -ENOMEM;
  }
  INIT_WORK(&pers->restart, restart_with_next_personality);
  pers->kset = kset_create_and_add("personality", /* ops */ 0, /* parent */ 0);
  error = usb_composite_register(&pers->driver);
  if (pers->gadget && !pers->personality) {
    usb_gadget_disconnect(pers->gadget); /* No personality selected: pullup(0) */
  }
  return error;
}
module_init(init);
static void __exit cleanup(void)
{
  struct personality* pers = instance();
  printk(KERN_INFO "personality cleanup\n");
  usb_composite_unregister(&pers->driver);
  kset_unregister(pers->kset);
  pers->kset = 0;
  if (pers->restart_worker) {
    flush_workqueue(pers->restart_worker);
    destroy_workqueue(pers->restart_worker);
  }
}
module_exit(cleanup);

struct kset * personality_cdev_to_kset(struct usb_composite_dev* cdev)
{
  struct personality * pers = cdev_to_personality(cdev);
  return pers ? pers->kset : 0;
}

struct usb_device_descriptor* personality_cdev_to_device_desc(struct usb_composite_dev* cdev)
{
  struct personality * pers = cdev_to_personality(cdev);
/* LGE_CHANGE_S kenneth.kang@lge.com 2011-01-16 USB patch */  
  return pers ? ((struct usb_device_descriptor*)pers->driver.dev) : 0;
/* LGE_CHANGE_E kenneth.kang@lge.com 2011-01-16 USB patch */
}

MODULE_AUTHOR("Paul Kunysch <paul.kunysch@emsys.de>");
MODULE_DESCRIPTION("Gadget driver with support for multiple personalities");
MODULE_LICENSE("GPL v2");

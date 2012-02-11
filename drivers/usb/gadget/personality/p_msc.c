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

#include "disable_init_defines.h"
#include "../f_mass_storage.c"

int mass_storage_function_add(struct usb_composite_dev *cdev, struct usb_configuration *c)
{
  return mass_storage_bind_config(c);
}

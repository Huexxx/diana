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
#include "../f_acm.c"
#include "../f_serial.c"

int acm_function_add(struct usb_configuration *c, u8 port_num)
{
  return acm_bind_config(c, port_num);
}

int gser_function_add(struct usb_configuration *c, u8 port_num)
{
  return gser_bind_config(c, port_num);
}


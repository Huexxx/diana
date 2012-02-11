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
#include "linux/init.h"

#undef __exit
#define __exit
#undef __exit_p
#define __exit_p(x) x

#undef __init
#define __init

#undef __initdata
#define __initdata

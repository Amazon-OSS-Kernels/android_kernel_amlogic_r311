/*
* Copyright (C) 2017 Amlogic, Inc. All rights reserved.
* *
This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* *
This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
* *
You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
* *
Description:
*/

#ifndef __ANTI_ROLLBACK_
#define __ANTI_ROLLBACK_

#include <linux/types.h>

bool check_antirollback(uint32_t kernel_version);

bool set_avb_antirollback(uint32_t index, uint32_t version);
bool get_avb_antirollback(uint32_t index, uint32_t* version);
bool get_avb_lock_state(uint32_t* lock_state);
bool avb_lock(void);
bool avb_unlock(void);

#endif

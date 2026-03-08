/*
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef _AD82587D_H
#define _AD82587D_H

#define MUTE                             0x02
#define MVOL                             0x03
#define C1VOL                            0x04
#define C2VOL                            0x05

#define AD82587D_REGISTER_COUNT          0x25
#define AD82587D_REGISTER_TABLE_SIZE     (AD82587D_REGISTER_COUNT * 2)

struct ad82587d_platform_data {
	int reset_pin;
};

#endif

/*
 * include/linux/amlogic/meson_cooldev.h
 *
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

#ifndef __MESON_COOLDEV_H__
#define __MESON_COOLDEV_H__

struct cool_dev {
	int min_state;
	int coeff;
	int gpupp;
	int cluster_id;
	u32 ddr_reg;
	u32 ddr_status;
	u32 ddr_bits[2];
	u32 ddr_data[16];
	u32 ddr_temp[16];
	char *device_type;
	struct device_node *np;
	struct thermal_cooling_device *cooling_dev;
};

#ifndef mc_capable
#define mc_capable()		0
#endif

struct thermal_cooling_device;
extern int meson_gcooldev_min_update(struct thermal_cooling_device *cdev);
#endif


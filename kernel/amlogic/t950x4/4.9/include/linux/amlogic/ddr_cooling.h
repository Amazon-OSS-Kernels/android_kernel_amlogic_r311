/*
 * include/linux/amlogic/ddr_cooling.h
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

#ifndef __DDR_COOLING_H__
#define __DDR_COOLING_H__

#include <linux/thermal.h>
#include <linux/amlogic/meson_cooldev.h>

struct ddr_cooling_device {
	int id;
	struct thermal_cooling_device *cool_dev;
	u32 ddr_reg;
	u32 ddr_status;
	u32 ddr_bits[2];
	u32 ddr_data[16];
	u32 ddr_temp[16];
	u32 ddr_bits_keep;	/*for keep ddr reg val excepts change bits*/
	struct list_head node;
};

#ifdef CONFIG_AMLOGIC_DDR_THERMAL

struct thermal_cooling_device *ddr_cooling_register(struct device_node *np,
						    struct cool_dev *cool);

/**
 * ddr_cooling_unregister - function to remove ddr cooling device.
 * @cdev: thermal cooling device pointer.
 */
void ddr_cooling_unregister(struct thermal_cooling_device *cdev);

#else

static inline struct thermal_cooling_device *
ddr_cooling_register(struct device_node *np,
		     struct cool_dev *cool)
{
	return NULL;
}

static inline
void ddr_cooling_unregister(struct thermal_cooling_device *cdev)
{
}
#endif

#endif /* __DDR_COOLING_H__ */

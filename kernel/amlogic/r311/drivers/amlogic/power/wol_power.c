/*
 * drivers/amlogic/ethernet/phy/wol_power.c
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

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kernel.h>

#include <linux/amlogic/scpi_protocol.h>


#define WOL_POWER_ENABLE  1
#define WOL_POWER_DISABLE 0


static unsigned int wol_power_state = WOL_POWER_DISABLE;


static int wol_power_proc_show(struct seq_file *seq, void *v)
{
	int scp_wol_state = WOL_POWER_DISABLE;

	scp_wol_state = scpi_get_wol_power();

	if ((scp_wol_state == 0) || (scp_wol_state == 1))
		wol_power_state = scp_wol_state;

	seq_printf(seq, "wol power state:%d\n", wol_power_state);
	return 0;
}

static ssize_t wol_power_proc_write(struct file *seq,
				const char __user *buffer,
				size_t count, loff_t *ppos)
{
	u32 value = 0;

	if (kstrtoint_from_user(buffer, count, 10, &value)) {
		pr_err("the value is invalid\n");
		return -EINVAL;
	}

	if ((value != WOL_POWER_ENABLE) && (value != WOL_POWER_DISABLE)) {
		pr_err("wol_en is 0 or 1, value=%d\n", value);
		return -EINVAL;
	}

	pr_info("set wol_power state=%d\n", value);
	if (scpi_set_wol_power(value) == 0)
		wol_power_state = value;

	return count;
}

static int wol_power_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, wol_power_proc_show, NULL);
}

static const struct file_operations  wol_power_fops = {
	.owner = THIS_MODULE,

	.open    = wol_power_proc_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release,
	.write   = wol_power_proc_write,
};


static int __init wol_power_init(void)
{
	proc_create("wol_enable", 0644, NULL, &wol_power_fops);
	return 0;
}

static int __init wol_power_setup(char *str)
{
	if (str != NULL)
		wol_power_state = (strcmp(str, "1") == 0);

	pr_info("wol_power_state:%d\n", wol_power_state);
	return scpi_set_wol_power(wol_power_state);
}

__setup("wol_power=", wol_power_setup);

fs_initcall(wol_power_init);

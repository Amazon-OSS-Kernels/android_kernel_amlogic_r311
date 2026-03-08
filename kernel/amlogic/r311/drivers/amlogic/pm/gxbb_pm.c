/*
 * Meson Power Management Routines
 *
 * Copyright (C) 2010 Amlogic, Inc. http://www.amlogic.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/pm.h>
#include <linux/suspend.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/spinlock.h>
#include <linux/clk.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>

#include <asm/cacheflush.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/amlogic/iomap.h>

#include <linux/init.h>
#include <linux/of.h>

#include <asm/compiler.h>
#include <linux/errno.h>
#include <asm/psci.h>
#include <linux/suspend.h>

#include <asm/suspend.h>
#include <linux/of_address.h>
#include <linux/input.h>
#include <linux/amlogic/pm.h>
#include <linux/amlogic/cpu_version.h>
#include <linux/amlogic/scpi_protocol.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static struct early_suspend early_suspend;
static int early_suspend_flag;
#endif

#undef pr_fmt
#define pr_fmt(fmt) "gxbb_pm: " fmt

static DEFINE_MUTEX(late_suspend_lock);
static LIST_HEAD(late_suspend_handlers);
static void __iomem *debug_reg;
static void __iomem *exit_reg;

#define POWERON_WAKE_LOCK "meson-poweron"
static int pm_wakeup_wakelock;
struct wakeup_source *wake_src;
#ifdef CONFIG_PROC_FS
static void get_poweron_reason(void);
#endif

static int suspend_count;

enum {
	DEBUG_USER_STATE = 1U << 0,
	DEBUG_SUSPEND = 1U << 2,
};
static int debug_mask = DEBUG_USER_STATE | DEBUG_SUSPEND;
struct late_suspend {
	struct list_head link;
	int level;
	void (*suspend)(struct late_suspend *h);
	void (*resume)(struct late_suspend *h);
	void *param;
};

void register_late_suspend(struct late_suspend *handler)
{
	struct list_head *pos;

	mutex_lock(&late_suspend_lock);
	list_for_each(pos, &late_suspend_handlers) {
		struct late_suspend *e;
		e = list_entry(pos, struct late_suspend, link);
		if (e->level > handler->level)
			break;
	}
	list_add_tail(&handler->link, pos);
	mutex_unlock(&late_suspend_lock);
}
EXPORT_SYMBOL(register_late_suspend);

void unregister_late_suspend(struct late_suspend *handler)
{
	mutex_lock(&late_suspend_lock);
	list_del(&handler->link);
	mutex_unlock(&late_suspend_lock);
}
EXPORT_SYMBOL(unregister_late_suspend);


static void late_suspend(void)
{
	struct late_suspend *pos;

	if (debug_mask & DEBUG_SUSPEND)
		pr_info("late_suspend: call handlers\n");
	list_for_each_entry(pos, &late_suspend_handlers, link) {
		if (pos->suspend != NULL) {
			pr_info("%pf\n", pos->suspend);
			pos->suspend(pos);
		}
	}

	if (debug_mask & DEBUG_SUSPEND)
		pr_info("late_suspend: sync\n");

}

static void early_resume(void)
{
	struct late_suspend *pos;

	if (debug_mask & DEBUG_SUSPEND)
		pr_info("late_resume: call handlers\n");
	list_for_each_entry_reverse(pos, &late_suspend_handlers, link)
	    if (pos->resume != NULL) {
		pr_info("%pf\n", pos->resume);
		pos->resume(pos);
	}

#ifdef CONFIG_PROC_FS
	get_poweron_reason();
#endif
	/* Hold wake lock for a while until Android really waking up*/
	if (pm_wakeup_wakelock)
		__pm_wakeup_event(wake_src, 5 * MSEC_PER_SEC);

	if (debug_mask & DEBUG_SUSPEND)
		pr_info("late_resume: done\n");
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void meson_system_early_suspend(struct early_suspend *h)
{
	if (!early_suspend_flag) {
		pr_info("%s\n", __func__);
		early_suspend_flag = 1;
	}
}

static void meson_system_late_resume(struct early_suspend *h)
{
	if (early_suspend_flag) {
		/* early_power_gate_switch(ON); */
		/* early_clk_switch(ON); */
		early_suspend_flag = 0;
		pr_info("%s\n", __func__);
	}
}
#endif

/*
 *0x10000 : bit[16]=1:control cpu suspend to power down
 *
 */
static void meson_gx_suspend(void)
{
	pr_info("enter meson_pm_suspend!\n");
	late_suspend();
#ifdef CONFIG_ARM64
	cpu_suspend(0x1);
#else
	/* TODO */
	cpu_suspend(0x1, NULL);
#endif
	early_resume();
	pr_info("... wake up\n");

}

static int meson_pm_prepare(void)
{
	pr_info("enter meson_pm_prepare!\n");
	return 0;
}

static int meson_gx_enter(suspend_state_t state)
{
	int ret = 0;
	switch (state) {
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		meson_gx_suspend();
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static void meson_pm_finish(void)
{
	pr_info("enter meson_pm_finish!\n");
}
unsigned int get_resume_method(void)
{
	unsigned int val = 0;
	if (exit_reg)
		val = readl(exit_reg);
	pr_info("%s reason = %#x, port = %#x, logic addr = %#x\n",
		__func__, (val & 0xFF), (val >> 16),
		(readl(exit_reg + 4) >> 16));

	if (val & 0xFF)
		suspend_count++;

	return val & 0xFF;
}
EXPORT_SYMBOL(get_resume_method);

static const struct platform_suspend_ops meson_gx_ops = {
	.enter = meson_gx_enter,
	.prepare = meson_pm_prepare,
	.finish = meson_pm_finish,
	.valid = suspend_valid_only_mem,
};

ssize_t suspend_count_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	int count;

	count = sprintf(buf, "%d\n", suspend_count);

	return count;
}

DEVICE_ATTR(suspend_count, S_IRUGO, suspend_count_show, NULL);

ssize_t time_out_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	unsigned val = 0, len;

	val = readl(debug_reg);

	len = sprintf(buf, "%d\n", val);

	return len;
}
ssize_t time_out_store(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
	unsigned time_out;
	int ret;

	ret = sscanf(buf, "%d", &time_out);

	switch (ret) {
	case 1:
		writel(time_out, debug_reg);
		break;
	default:
		return -EINVAL;
	}

	return count;
}


DEVICE_ATTR(time_out, 0660, time_out_show, time_out_store);

static int suspend_reason;
ssize_t suspend_reason_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	unsigned  len;

	len = sprintf(buf, "%d\n", suspend_reason);

	return len;
}
ssize_t suspend_reason_store(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t count)
{
	int ret;

	ret = sscanf(buf, "%d", &suspend_reason);

	switch (ret) {
	case 1:
		__invoke_psci_fn_smc(0x82000042, suspend_reason, 0, 0);
		break;
	default:
		return -EINVAL;
	}
	return count;
}

DEVICE_ATTR(suspend_reason, 0660, suspend_reason_show, suspend_reason_store);

#ifdef CONFIG_PROC_FS
#define POWERON_REASON_NAME "meson-poweron-reason"
static char power_on_src[32] = {};
static void get_poweron_reason(void)
{
	unsigned int power_on_method;
	strcpy(power_on_src, "unknown");

	pm_wakeup_wakelock = 0;
	power_on_method = get_resume_method();
	switch (power_on_method) {
	case REMOTE_WAKEUP:
	case REMOTE_CUS_WAKEUP:
	case POWER_KEY_WAKEUP:
		sprintf(power_on_src, "%s", "power");
		break;

	case WIFI_WAKEUP:
		sprintf(power_on_src, "%s", "WOW");
		pm_wakeup_wakelock = 1;
		break;

	case CEC_WAKEUP:
		sprintf(power_on_src, "%s", "CEC");
		pm_wakeup_wakelock = 1;
		break;

	case ETH_PHY_WAKEUP:
		sprintf(power_on_src, "%s", "WOL");
		pm_wakeup_wakelock = 1;
		break;

	case REMOTE_CUSTOM1_WAKEUP:
		sprintf(power_on_src, "%s", "app_1");
		pm_wakeup_wakelock = 1;
		break;

	case REMOTE_CUSTOM2_WAKEUP:
		sprintf(power_on_src, "%s", "app_2");
		pm_wakeup_wakelock = 1;
		break;

	case REMOTE_CUSTOM3_WAKEUP:
		sprintf(power_on_src, "%s", "app_3");
		pm_wakeup_wakelock = 1;
		break;

	case REMOTE_CUSTOM4_WAKEUP:
		sprintf(power_on_src, "%s", "app_4");
		pm_wakeup_wakelock = 1;
		break;

	default:
		sprintf(power_on_src, "%s", "unknown");
		pr_warn("power_on_method=%d is unknown\n", power_on_method);
		break;
	}

	pr_info("system is powered on by %s\n", power_on_src);

	return;
}

static int show_power_on_reason(struct seq_file *m, void *v)
{
	pr_info("system is powered on by %s\n", power_on_src);
	seq_printf(m, "%s", power_on_src);
	return 0;
}

static int meson_poweron_reason_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, show_power_on_reason, NULL);
}

static const struct file_operations meson_poweron_reason_fops = {
	.read = seq_read,
	.open = meson_poweron_reason_open,
	.release = single_release,
};
#endif

static int __init meson_pm_probe(struct platform_device *pdev)
{
	pr_info("enter meson_pm_probe!\n");
#ifdef CONFIG_HAS_EARLYSUSPEND
	early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	early_suspend.suspend = meson_system_early_suspend;
	early_suspend.resume = meson_system_late_resume;
	register_early_suspend(&early_suspend);
#endif

	if (of_property_read_bool(pdev->dev.of_node, "gxbaby-suspend"))
		suspend_set_ops(&meson_gx_ops);

	debug_reg = of_iomap(pdev->dev.of_node, 0);
	exit_reg = of_iomap(pdev->dev.of_node, 1);
	writel(0x0, debug_reg);
	device_create_file(&pdev->dev, &dev_attr_time_out);
	device_create_file(&pdev->dev, &dev_attr_suspend_reason);
	device_create_file(&pdev->dev, &dev_attr_suspend_count);

	wake_src = wakeup_source_register(POWERON_WAKE_LOCK);
	if (!wake_src) {
#ifdef CONFIG_HAS_EARLYSUSPEND
		unregister_early_suspend(&early_suspend);
#endif
		return -ENOMEM;
	}

#ifdef CONFIG_PROC_FS
	get_poweron_reason();
	proc_create(POWERON_REASON_NAME,
		S_IRUSR, NULL, &meson_poweron_reason_fops);
#endif
	if (scpi_clr_wakeup_reason())
		pr_debug("clr wakeup reason fail.\n");

	pr_info("meson_pm_probe done\n");
	return 0;
}

static int __exit meson_pm_remove(struct platform_device *pdev)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&early_suspend);
#endif
	wakeup_source_unregister(wake_src);
	suspend_count = 0;

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id amlogic_pm_dt_match[] = {
	{.compatible = "amlogic, pm",
	 },
	{},
};
#else
#define amlogic_nand_dt_match NULL
#endif

static struct platform_driver meson_pm_driver = {
	.driver = {
		   .name = "pm-meson",
		   .owner = THIS_MODULE,
		   .of_match_table = amlogic_pm_dt_match,
		   },
	.remove = __exit_p(meson_pm_remove),
};

static int __init meson_pm_init(void)
{
	suspend_count = 0;
	return platform_driver_probe(&meson_pm_driver, meson_pm_probe);
}

late_initcall(meson_pm_init);

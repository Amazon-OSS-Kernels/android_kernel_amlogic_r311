/*
 * drivers/amlogic/reset/aml-poweroff.c
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
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


#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/reboot.h>

#include <asm/system_misc.h>

#include <linux/amlogic/iomap.h>
#include <linux/amlogic/cpu_version.h>
#include <linux/compiler.h>
#include <linux/kdebug.h>
#ifndef CONFIG_ARM64
#include <asm/opcodes-sec.h>
#endif
/*
 * Commands accepted by the arm_machine_restart() system call.
 *
 * MESON_NORMAL_BOOT			Restart system normally.
 * MESON_FACTORY_RESET_REBOOT      Restart system into recovery factory reset.
 * MESON_UPDATE_REBOOT			Restart system into recovery update.
 * MESON_CHARGING_REBOOT		Restart system into charging.
 * MESON_CRASH_REBOOT			Restart system with system crach.
 * MESON_FACTORY_TEST_REBOOT	Restart system into factory test.
 * MESON_SYSTEM_SWITCH_REBOOT	Restart system for switch other OS.
 * MESON_SAFE_REBOOT			Restart system into safe mode.
 * MESON_LOCK_REBOOT			Restart system into lock mode.
 */
/*******************************************************************/
#define	MESONTXLX_COLD_REBOOT					0x0
#define	MESONTXLX_NORMAL_BOOT					0x1
#define	MESONTXLX_FACTORY_RESET_REBOOT				0x2
#define	MESONTXLX_UPDATE_REBOOT					0x3
#define	MESONTXLX_FASTBOOT_REBOOT				0x4
#define MESONTXLX_UBOOT_SUSPEND					0x5
#define MESONTXLX_HIBERNATE					0x6
#define MESONTXLX_BOOTLOADER_REBOOT				7
#define MESONTXLX_RPMBP_REBOOT					0x9
#define	MESONTXLX_CRASH_REBOOT					11
#define	MESONTXLX_KERNEL_PANIC					12


#define AO_RTI_STATUS_REG1		((0x00 << 10) | (0x01 << 2))
#define WATCHDOG_TC			0x2640
#define EFUSE_ENABLE_ANTIROLLBACK	0x82000048

static u32 psci_function_id_restart;
static u32 psci_function_id_poweroff;
static char *kernel_panic;
static int __invoke_psci_fn_smc(u64 function_id, u64 arg0, u64 arg1, u64 arg2);

static u32 parse_reason(const char *cmd)
{
	u32 reboot_reason;
	reboot_reason = MESONTXLX_NORMAL_BOOT;
	if (cmd) {
		if (strcmp(cmd, "recovery") == 0 ||
				strcmp(cmd, "factory_reset") == 0)
			reboot_reason = MESONTXLX_FACTORY_RESET_REBOOT;
		else if (strcmp(cmd, "update") == 0)
			reboot_reason = MESONTXLX_UPDATE_REBOOT;
		else if (strcmp(cmd, "fastboot") == 0)
			reboot_reason = MESONTXLX_FASTBOOT_REBOOT;
		else if (strcmp(cmd, "bootloader") == 0)
			reboot_reason = MESONTXLX_FASTBOOT_REBOOT;
		else if (strcmp(cmd, "report_crash") == 0)
			reboot_reason = MESONTXLX_CRASH_REBOOT;
		else if (strcmp(cmd, "uboot_suspend") == 0)
			reboot_reason = MESONTXLX_UBOOT_SUSPEND;
		else if (strcmp(cmd, "rpmbp") == 0)
			reboot_reason = MESONTXLX_RPMBP_REBOOT;
		else if (strcmp(cmd, "arb_enable") == 0)
			__invoke_psci_fn_smc(EFUSE_ENABLE_ANTIROLLBACK,
				(('a' << 16) | ('r' << 8) | ('b' << 0)), 0, 0);
	} else {
		if (kernel_panic) {
			if (strcmp(kernel_panic, "kernel_panic") == 0)
				reboot_reason = MESONTXLX_KERNEL_PANIC;
		}

	}

	pr_info("reboot reason %d\n", reboot_reason);
	return reboot_reason;
}

#ifdef CONFIG_ARM64
#define __asmeq(x, y)  ".ifnc " x "," y " ; .err ; .endif\n\t"
static noinline int __invoke_psci_fn_smc(u64 function_id, u64 arg0, u64 arg1,
					 u64 arg2)
{
	register long x0 asm("x0") = function_id;
	register long x1 asm("x1") = arg0;
	register long x2 asm("x2") = arg1;
	register long x3 asm("x3") = arg2;
	asm volatile(
			__asmeq("%0", "x0")
			__asmeq("%1", "x1")
			__asmeq("%2", "x2")
			__asmeq("%3", "x3")
			"smc	#0\n"
		: "+r" (x0)
		: "r" (x1), "r" (x2), "r" (x3));

	return function_id;
}
#else
static noinline int __invoke_psci_fn_smc(u64 function_id, u64 arg0, u64 arg1,
					 u64 arg2)
{
	register int r0 asm("r0") = function_id;
	register int r1 asm("r1") = arg0;
	register int r2 asm("r2") = arg1;
	register int r3 asm("r3") = arg2;
	asm volatile(
			__asmeq("%0", "r0")
			__asmeq("%1", "r1")
			__asmeq("%2", "r2")
			__asmeq("%3", "r3")
			__SMC(0)
		: "+r" (r0)
		: "r" (r1), "r" (r2), "r" (r3));

	return function_id;
}
#endif

void meson_smc_restart(u64 function_id, u64 reboot_reason)
{
	__invoke_psci_fn_smc(function_id,
				reboot_reason, 0, 0);
}
void meson_common_restart(char mode, const char *cmd)
{
	u32 reboot_reason = parse_reason(cmd);
	if (psci_function_id_restart)
		meson_smc_restart((u64)psci_function_id_restart,
						(u64)reboot_reason);
}
static void do_aml_restart(enum reboot_mode reboot_mode, const char *cmd)
{
	meson_common_restart(reboot_mode, cmd);
}

static void do_aml_poweroff(void)
{
	/* TODO: Add poweroff capability */
	__invoke_psci_fn_smc(0x82000042, 1, 0, 0);
	__invoke_psci_fn_smc(psci_function_id_poweroff,
				0, 0, 0);
}
static int panic_notify(struct notifier_block *self,
			unsigned long cmd, void *ptr)
{
	kernel_panic = "kernel_panic";

	return NOTIFY_DONE;
}

static struct notifier_block panic_notifier = {
	.notifier_call	= panic_notify,
};

static int aml_restart_probe(struct platform_device *pdev)
{
	u32 id;
	int ret;
	pm_power_off = do_aml_poweroff;
	arm_pm_restart = do_aml_restart;

	if (!of_property_read_u32(pdev->dev.of_node, "sys_reset", &id))
		psci_function_id_restart = id;

	if (!of_property_read_u32(pdev->dev.of_node, "sys_poweroff", &id))
		psci_function_id_poweroff = id;
	ret = register_die_notifier(&panic_notifier);
	if (ret != 0)
			return ret;

	return 0;
}

static const struct of_device_id of_aml_restart_match[] = {
	{ .compatible = "aml, restart", },
	{},
};
MODULE_DEVICE_TABLE(of, of_aml_restart_match);

static struct platform_driver aml_restart_driver = {
	.probe = aml_restart_probe,
	.driver = {
		.name = "aml-restart",
		.of_match_table = of_match_ptr(of_aml_restart_match),
	},
};

static int __init aml_restart_init(void)
{
	return platform_driver_register(&aml_restart_driver);
}
device_initcall(aml_restart_init);

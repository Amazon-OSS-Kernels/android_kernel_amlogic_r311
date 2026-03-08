/*
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

/* #define	DEBUG	*/

#include <common.h>
#include <autoboot.h>
#include <cli.h>
#include <version.h>
#include <asm/arch/timer.h>
#include <asm/arch/secure_apb.h>

#ifdef CONFIG_MDUMP_COMPRESS
#include <ramdump.h>
#endif

#if defined(UFBL_FEATURE_FASTBOOT_LOCKDOWN)
#include "amzn_fastboot_lockdown.h"
extern int is_locked_production_device();
#else
#error "UFBL_FEATURE_FASTBOOT_LOCKDOWN is required"
#endif

#ifdef CONFIG_IDME
#include <idme.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#if defined(BL33_BOOT_TIME_PROBE)
	#define TE TE_time
#else
	#define TE(...)
#endif

#if defined (UBOOT_TARGET_PRODUCT_NAME_ALMOND) || defined (UBOOT_TARGET_PRODUCT_NAME_ABC) || defined (UBOOT_TARGET_PRODUCT_NAME_DAHLIA) || defined (UBOOT_TARGET_PRODUCT_NAME_ABC)
static u32 fb_width;
static u32 fb_height;
static u32 display_bpp;
static unsigned char *fb_addr;
extern unsigned long get_fb_addr(void);

#define COLOR_RED_24BPP (0xFF << 16)
#define COLOR_GREEN_24BPP (0xFF << 8)
#define COLOR_BLUE_24BPP (0xFF << 0)
#define COLOR_WHITE_24BPP (COLOR_RED_24BPP | COLOR_GREEN_24BPP | COLOR_BLUE_24BPP)

#define COLOR_RED_16BPP (0x1F <<11)
#define COLOR_GREEN_16BPP (0x3F << 5)
#define COLOR_BLUE_16BPP (0x1F << 0)
#define COLOR_WHITE_16BPP (COLOR_RED_16BPP | COLOR_GREEN_16BPP | COLOR_BLUE_16BPP)



static int show_transition_color_block(u32 color, int pos_x, int pos_y, int w, int h)
{
	int i, j;
	unsigned char *fbp = NULL;
	int byte_per_pixel;

	if (!fb_addr) {
		printf("Framebuffer wasn't initialized\n");
		goto error;
	}

	if (pos_x < 0 || w < 0 ||
		pos_y < 0 || h < 0 ||
		(pos_x + w) > fb_width ||
		(pos_y + h) > fb_height) {
			printf("Position is out of range, image upgrade failed\n");
			goto error;
	}

	byte_per_pixel = display_bpp / 8;
	fbp = fb_addr + (fb_width * pos_y + pos_x) * byte_per_pixel;

	for (i = 0; i < h ; i++) {
		for (j = 0; j < w; j++) {
			*(fbp + 0) = color & 0xFF;
			*(fbp + 1) = (color >> 8) & 0xFF;
			if (24 == display_bpp) {
				*(fbp + 2) = (color >> 16) & 0xFF;
			}
			fbp += byte_per_pixel;
		}
		fbp += (fb_width - w) * byte_per_pixel;
	}

	flush_cache(fb_addr, fb_width * fb_height * byte_per_pixel);

	return 0;
error:
	return -1;
}


static int show_transition_done(int cur_step, int total_steps, int result)
{
	int x, y, w, h, color;
	const int progress_bar_x = 100;
	const int progress_bar_y = 400;
	const int progress_bar_w = fb_width - 200;
	const int progress_bar_h = 400;

	if (cur_step < 0 || total_steps < 0 || cur_step > total_steps ||
			(cur_step != 0 && total_steps == 0))
		goto error;

	x = progress_bar_x;
	y = progress_bar_y;
	h = progress_bar_h;
	w = !(cur_step | total_steps) ? progress_bar_w :
		progress_bar_w * cur_step / total_steps;

	if (24 == display_bpp) {
		color = !(cur_step | total_steps) ? COLOR_WHITE_24BPP:
		result ? COLOR_RED_24BPP : COLOR_GREEN_24BPP;
	} else if (16 == display_bpp) {
		color = !(cur_step | total_steps) ? COLOR_WHITE_16BPP:
		result ? COLOR_RED_16BPP : COLOR_GREEN_16BPP;

	}

	show_transition_color_block(color, x, y, w, h);

	return 0;
error:
	return -1;
}

static int transition_done_ui_init(void)
{
	char *str = NULL;
	fb_addr = (unsigned char *)get_fb_addr();

	str = getenv("fb_width");
	fb_width = str ? simple_strtoul(str, NULL, 10) : 0;

	str = getenv("fb_height");
	fb_height = str ? simple_strtoul(str, NULL, 10) : 0;

	str = getenv("display_bpp");
	display_bpp = str ? simple_strtoul(str, NULL, 10) : 0;

	if (!(fb_width && fb_width && display_bpp && fb_addr))
		goto error;
	return 0;
error:
	return -1;
}
#endif

/*
 * Board-specific Platform code can reimplement show_boot_progress () if needed
 */
__weak void show_boot_progress(int val) {}

static void modem_init(void)
{
#ifdef CONFIG_MODEM_SUPPORT
	debug("DEBUG: main_loop:   gd->do_mdm_init=%lu\n", gd->do_mdm_init);
	if (gd->do_mdm_init) {
		char *str = getenv("mdm_cmd");

		setenv("preboot", str);  /* set or delete definition */
		mdm_init(); /* wait for modem connection */
	}
#endif  /* CONFIG_MODEM_SUPPORT */
}

static void run_preboot_environment_command(void)
{
#ifdef CONFIG_PREBOOT
	char *p;

	p = getenv("preboot");
	if (p != NULL) {
# ifdef CONFIG_AUTOBOOT_KEYED
		int prev = disable_ctrlc(1);	/* disable Control C checking */
# endif
		TE("preboot");
		run_command_list(p, -1, 0);
		TE("preboot");

# ifdef CONFIG_AUTOBOOT_KEYED
		disable_ctrlc(prev);	/* restore Control C checking */
# endif
	}
#endif /* CONFIG_PREBOOT */
}


/*
AMAZON hope to break the commnad and enter fastboot
*/
static int break_command(void) {
	int key = 0;

	// CHECK USER Input in uart
	if (tstc()) {
		key = getc();

		// the Ctrl+b;
		if (key == 0x02)
			return 1;/* Break Command */
	}

	return 0;
}



/* We come here after U-Boot is initialised and ready to process commands */
void main_loop(void)
{
	const char *s;
#ifdef CONFIG_IDME
	int ret = -1;
	int is_diag_bootmode = 0;
	int is_transition_done = 1;
	int is_transition_bootmode = 0;
	int bootmode = IDME_BOOTMODE_NORMAL;
	char transition_str[8] = {0};
#ifdef DTB_BIND_KERNEL
	unsigned char *dt_addr = NULL;
	extern int emmc_update_mbr(unsigned char *);
	char oem_data[64] = {0};
#endif /* DTB_BIND_KERNEL */
#endif

	bootstage_mark_name(BOOTSTAGE_ID_MAIN_LOOP, "main_loop");
#ifdef CONFIG_MDUMP_COMPRESS
	ramdump_init();
#endif

#ifndef CONFIG_SYS_GENERIC_BOARD
	puts("Warning: Your board does not use generic board. Please read\n");
	puts("doc/README.generic-board and take action. Boards not\n");
	puts("upgraded by the late 2014 may break or be removed.\n");
#endif

	modem_init();
#ifdef CONFIG_VERSION_VARIABLE
	setenv("ver", version_string);  /* set version variable */
#endif /* CONFIG_VERSION_VARIABLE */

	cli_init();


#ifdef CONFIG_IDME
	bootmode = idme_boot_mode();
	is_diag_bootmode = (bootmode == IDME_BOOTMODE_DIAG);
#if defined(UBOOT_TARGET_PRODUCT_NAME_ALMOND) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined (UBOOT_TARGET_PRODUCT_NAME_DAHLIA) || defined (UBOOT_TARGET_PRODUCT_NAME_ABC)
	is_transition_bootmode = ((bootmode == IDME_BOOTMODE_TRANSITION) || (bootmode == IDME_BOOTMODE_STANDBY_LOGO_POST_SHIPPING_SW_SWITCH));
#else
	is_transition_bootmode = (bootmode == IDME_BOOTMODE_TRANSITION);
#endif

	if (idme_get_var_external("transition_done", transition_str, sizeof(transition_str)) == 0)
		is_transition_done = !strncmp(transition_str, "1", 1);
	else
		printf("Get transition_done value failed\n");

	printf("transition_done=%d\n", is_transition_done);

	if (is_transition_bootmode) {
		if (!is_transition_done) {
			ret = 0;
			idme_get_var_external("oem_data", oem_data, sizeof(oem_data));
			if (strstr(oem_data, "ABC") != NULL || strstr(oem_data, "ABC-tm") != NULL || strstr(oem_data, "ABC-jb") != NULL || strstr(oem_data, "ABC-jm") != NULL || strstr(oem_data, "almondmo") != NULL || strstr(oem_data, "almondh") != NULL || strstr(oem_data, "almondb") != NULL) {
				ret += run_command("amlmmc erase dfs", 0);
				ret += run_command("amlmmc erase dkernel", 0);
				ret += run_command("amlmmc erase diag_userdata", 0);
				ret += run_command("amlmmc erase dvendor", 0);
				if (ret)
					printf("Erase Diag partitions error\n");
			}
			ret = ret ? ret : run_command("idme bootmode 1", 0);
			if (ret) {
				printf("Change to FOS bootmode error\n");
				printf("Transition from Diag to FOS failed\n");
			}

#ifdef DTB_BIND_KERNEL
#ifdef CONFIG_DTB_MEM_ADDR
			dt_addr = (unsigned char *)CONFIG_DTB_MEM_ADDR;
#else
			dt_addr = (unsigned char *)0x01000000;
#endif
			printf("Transit to FOS MBR\n");
			ret = ret ? ret : emmc_update_mbr(dt_addr);
			if (ret)
				printf("Update MBR failed\n");
#endif /* DTB_BIND_KERNEL */
			ret = ret ? ret : run_command("idme transition_done 1", 0);
			if (ret) {
				printf("Set transition_done error\n");
				printf("Transition from Diag to FOS failed\n");
			} else {
				printf("Transition from Diag to FOS succeed\n");
				if ((strstr(oem_data, "almond") != NULL && strstr(oem_data, "almondmo") == NULL && strstr(oem_data, "almondh") == NULL && strstr(oem_data, "almondb") == NULL) || strstr(oem_data, "ABC-jt") != NULL || strstr(oem_data, "dahlia") != NULL) {
					if (bootmode == IDME_BOOTMODE_TRANSITION) {
						run_command("reboot", 0);
					} else {
						printf("Transition to FOS standby mode");
						run_command("osd open;osd clear;logo_display $logo_name;bmp scale;vout output ${outputmode};", 0);

						ret = 0;
						ret = transition_done_ui_init();
						if (ret < 0) {
							printf("Transition done GUI init failure\n");
						} else {
							int AML_RPMB_STATE = ((readl(AO_SEC_GP_CFG7))>>22) & 0x01;
							show_transition_done(1, 1, !(AML_RPMB_STATE));
							run_command("leds_state 0 2 4", 0);
							printf("Transition done GUI init done\n");
						}
					while (1)
						udelay(1000*1000);
					}
				} else if (strstr(oem_data, "ABC") != NULL || strstr(oem_data, "ABC-tm") || strstr(oem_data, "ABC-jb") || strstr(oem_data, "ABC-jm") || strstr(oem_data, "almondmo") || strstr(oem_data, "almondh") || strstr(oem_data, "almondb")) {
					ret = 0;
					ret += run_command("osd open", 0);
					ret += run_command("osd clear", 0);
					/* mmc 1:d tvconfig, fixed */
					if (amzn_target_is_lockdown()) {
						//locked down transition
						ret += run_command("ext4load mmc 1:d ${loadaddr} /logo_img_files/transition_done.bmp", 0);
					} else {
						//unlocked transition
						ret += run_command("ext4load mmc 1:d ${loadaddr} /logo_img_files/transition_done_red.bmp", 0);
					}
					if (ret == 0) {
						ret += run_command("bmp display ${loadaddr}", 0);
						ret += run_command("bmp scale", 0);
						run_command("vout output ${outputmode}", 0);
					} else {
						printf("Cannot load transition bmp.\n");
					}
					watchdog_disable();
					printf("Stop after transition from diag to FOS\n");
					while (1) {
						udelay(1000*1000);
					}
				} else if (strstr(oem_data, "ABC-vv") || strstr(oem_data, "ABC-vp")) {
					watchdog_disable();
					printf("\n\nshipment mode operations finished\n\n");
					while (1) {
						udelay(1000*1000);
					}
				} else
					run_command("reboot", 0);
			}
		} else {
			printf("Transition had already been done, stop transitting\n");
		}
	} else if (is_diag_bootmode) {
		printf("bootmode is Diag, will change boot_part to dkernel\n");
		if (is_transition_done) {
			printf("Switched back to Diag, restore transition status\n");
			ret = run_command("idme transition_done 0", 0);
			if (ret)
				printf("Set transition_done error\n");
		}
		ret = setenv("boot_part", "dkernel");
		if (ret)
			printf("Change boot_part to dkernel error\n");
	}

	char dev_flags_str[19] = { 0 };
	unsigned long long dev_flags = 0;

	if(0 != idme_get_var_external("dev_flags", dev_flags_str, 16)) {
		printf("Error getting dev_flags value\n");
	} else {
		dev_flags = simple_strtoul(dev_flags_str, NULL, 16);
		if ((dev_flags & DEV_FLAGS_SELINUX_FORCE_ENFORCING) == DEV_FLAGS_SELINUX_FORCE_ENFORCING) {
			printf("Force selinux enforcing mode\n");
			setenv("EnableSelinux", "enforcing");

		} else if ((dev_flags & DEV_FLAGS_SELINUX_FORCE_PERMISSIVE) == DEV_FLAGS_SELINUX_FORCE_PERMISSIVE) {
			printf("Force selinux permissive mode\n");
			setenv("EnableSelinux", "permissive");
		}
#if (defined (UBOOT_TARGET_PRODUCT_NAME_ALMOND) || defined (UBOOT_TARGET_PRODUCT_NAME_ABC) || defined (UBOOT_TARGET_PRODUCT_NAME_DAHLIA) || defined (UBOOT_TARGET_PRODUCT_NAME_ABC)) && defined(UFBL_FEATURE_FASTBOOT_LOCKDOWN)
		if ((dev_flags & DEV_FLAGS_ENABLE_FACTORY_TEST ) == DEV_FLAGS_ENABLE_FACTORY_TEST
				&& bootmode == IDME_BOOTMODE_NORMAL && is_locked_production_device()) {
			printf("factory test mode and arb enabled, switch to diag bootmode\n");
			ret = run_command("idme bootmode 2", 0);
			if (ret) {
				printf("Change to diag bootmode error\n");
			}
			run_command("reboot", 0);
		}
#endif
	}

#endif /* CONFIG_IDME */

	if (break_command()) {
		printf("***Force enter fastboot, must REBOOT\n");
		run_command("mw ff638630 0 2", 0);

		/*
		Init LCD. it can avoid the board enter standby ,when use fastboot reboot command.
		*/
		run_command("run init_display", 0);
		run_command("fastboot", 0);

		/*
		exit fastboot, reset.
		*/
		do_reset(NULL, 0, 0, NULL);
	} else
		run_preboot_environment_command();

#if defined(CONFIG_UPDATE_TFTP)
	update_tftp(0UL);
#endif /* CONFIG_UPDATE_TFTP */

	s = bootdelay_process();
	if (cli_process_fdt(&s))
		cli_secure_boot_cmd(s);

#if defined(CONFIG_AML_UBOOT_AUTO_TEST)
	//stick 0 and stick 1 will be used to check the boot process of uboot
	//stick 0 is the start counter (0xC8834400 + 0x7C<<2) = 0xc88345f0
	//stick 1 is the end counter   (0xC8834400 + 0x7D<<2) = 0xc88345f4
	if (*((volatile unsigned int*)(0xc88345f0)))
	{
		printf("\n\naml log : TE = %d\n",*((volatile unsigned int*)0xc1109988));
		*((volatile unsigned int*)(0xc88345f4)) += 1; //stick 1
		printf("\n\naml log : Boot success %d times @ %d\n",*((volatile unsigned int*)(0xc88345f4)),
			*((volatile unsigned int*)(0xc88345f0))); //stick 0 set in bl2_main.c
		int ndelay = 10;
		int nabort = 0;
		while (ndelay)
		{
			udelay(1);
			if (tstc())
			switch (getc())
			{
			//case 0x20: /* Space */
			case 0x0d: /* Enter */
				nabort = 1;
				break;
			}
			ndelay -= 1;
		}
		if (!nabort)
			run_command("reset",0);
	}
#endif //#if defined(CONFIG_AML_UBOOT_AUTO_TEST)

	autoboot_command(s);

	watchdog_disable();
	printf("close watchdog\n");

#if defined(UFBL_FEATURE_FASTBOOT_LOCKDOWN)
	/* We are hitting interactive prompt, begin commands lock down */
	amzn_block_commands();
#endif
	cli_loop();
}

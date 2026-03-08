/*
 * board/amlogic/configs/blanche.h
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#ifndef __BLANCHE_H__
#define __BLANCHE_H__

#include <asm/arch/cpu.h>

#define CONFIG_SYS_GENERIC_BOARD	1
#ifndef CONFIG_AML_MESON
#warning "include warning"
#endif

#define CONFIG_SYS_VSNPRINTF 1

/*
 * platform power init config
 */
#define CONFIG_PLATFORM_POWER_INIT
#define CONFIG_VCCK_INIT_VOLTAGE	1050	/* 1100 */
#define CONFIG_VDDEE_INIT_VOLTAGE	960	/* voltage for power up */
#define CONFIG_VDDEE_INIT_VOLTAGE_DDR3	980	/*voltage for power up for DDR3*/
#define CONFIG_VDDEE_SLEEP_VOLTAGE	900	/* voltage for suspend */

/* configs for CEC */
#define CONFIG_CEC_OSD_NAME		"AML_TV"
#define CONFIG_CEC_WAKEUP

/* configs for wakeup of WiFi and BT */
/* #define CONFIG_BT_WAKEUP */
#define CONFIG_WIFI_WAKEUP

/* SMP Definitinos */
#define CPU_RELEASE_ADDR		secondary_boot_func

/* config saradc*/
#define CONFIG_CMD_SARADC		1

/*
 * Bootloader Control Block function
 * That is used for recovery and the bootloader to talk to each other
 */
#define CONFIG_BOOTLOADER_CONTROL_BLOCK

/* support uboot usb update*/
#define CONFIG_UBOOT_USB_UPDATE

#define CONFIG_CMD_EXT4			1

/* Serial config */
#define CONFIG_CONS_INDEX		2
#define CONFIG_BAUDRATE			460800
#define CONFIG_AML_MESON_SERIAL		1
#define CONFIG_SERIAL_MULTI		1

#if  defined UBOOT_TARGET_PRODUCT_NAME_ANJALI
#define CONFIG_AUTO_COMPLETE            1
#define CONFIG_CMDLINE_EDITING          1
#endif

/* Enable ir remote wake up for bl30 */
#define CONFIG_IR_REMOTE_POWER_UP_KEY_CNT	3
#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL1	0xef10fe01 /* amlogic tv ir --- power */
#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL2	0XBB44FB04 /* amlogic tv ir --- ch+ */
#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL3	0xF20DFE01 /* amlogic tv ir --- ch- */
#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL4	0xFFFFFFFF
#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL5	0xe51afb04

/*https://amazon.com*/

#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL6	0xb9467d02 /* earhart power key */
#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL7	0xa05f7d02 /* earhart netflix key */
#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL8	0x5ea17d02 /* earhart prime video key */
#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL9	0x5da27d02 /* earhart music key */
#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL10	0x5ca37d02 /* earhart custom button4 key */
#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL11	0x609f7d02 /* earhart home key */
#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL12	0xb54a7d02 /* earhart enter key */
#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL13	0x5fa07d02 /* earhart voice search key */

#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL14	0xed12bf40 /* Toshiba remote --- power */
#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL15	0xf00f0586 /* Insignia remote --- power */

#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL16	0x9e610586 /* Insignia additional remote --- power */
#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL17	0x817EBF40 /* Toshiba additional remote --- power */

#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL18	0x5ba47d02 /* ABS presetting1 key */
#define CONFIG_IR_REMOTE_POWER_UP_KEY_VAL19	0x5aa57d02 /* ABS presetting2 key */

/* config the default parameters for adc power key */
#define CONFIG_ADC_POWER_KEY_CHAN		2  /* channel range: 0-7*/
#define CONFIG_ADC_POWER_KEY_VAL		0  /* sample value range: 0-1023*/

/* args/envs */
#define CONFIG_SYS_MAXARGS			64
#define CONFIG_EXTRA_ENV_SETTINGS \
        "firstboot=1\0"\
        "upgrade_step=0\0"\
        "jtag=apee\0"\
        "loadaddr=1080000\0"\
        "panel_type=lvds_2\0" \
        "outputmode=768p60hz\0" \
        "hdmimode=1080p60hz\0" \
        "cvbsmode=576cvbs\0" \
        "display_width=1920\0" \
        "display_height=1080\0" \
        "display_bpp=24\0" \
        "display_color_index=24\0" \
        "display_layer=osd1\0" \
        "display_color_fg=0xffffffff\0" \
        "display_color_bg=0\0" \
        "dtb_mem_addr=0x1000000\0" \
        "fb_addr=0x3d800000\0" \
        "fb_width=1920\0" \
        "fb_height=1080\0" \
        "fastboot_burning=fastboot\0" \
        "fdt_high=0x20000000\0"\
        "try_auto_burn=update 700 750;\0"\
        "sdcburncfg=aml_sdc_burn.ini\0"\
        "sdc_burning=sdc_burn ${sdcburncfg}\0"\
        "wipe_data=successful\0"\
        "wipe_cache=successful\0"\
        "EnableSelinux=enforcing\0" \
        "recovery_part=recovery\0"\
        "recovery_offset=0\0"\
        "cvbs_drv=0\0"\
        "osd_reverse=all,true\0"\
        "video_reverse=1\0"\
        "bl_level=255\0"\
        "bl_status=1\0"\
        "silent_ota=0\0"\
        "display_is_running=0\0"\
        "reboot_mode=cold_boot\0"\
        "active_slot=_a\0"\
        "boot_part=boot\0"\
        "rpmb_state=0\0"\
        "hwid=0\0"\
        "model_name=" DEFAULT_MODEL_INI "\0"\
        "transition_done=0\0"\
        "bypass_standby=0\0"\
        "logo_fn=bootup\0"\
        "initargs="\
            "rootfstype=ramfs init=/init console=ttyS0,460800 no_console_suspend earlyprintk=aml-uart,0xff803000 ramoops.pstore_en=1 ramoops.record_size=0x8000 ramoops.console_size=0x20000 "\
            "\0"\
        "upgrade_check="\
            "echo upgrade_step=${upgrade_step}; "\
            "if itest ${upgrade_step} == 3; then "\
                "run init_display; run storeargs; run update;"\
            "else fi;"\
            "\0"\
        "storeargs="\
            "memory_info ;"\
            "setenv bootargs ${initargs} logo=${display_layer},loaded,${fb_addr} vout=${outputmode},enable panel_type=${panel_type} osd_reverse=${osd_reverse} video_reverse=${video_reverse} bl_level=${bl_level} bl_status=${bl_status} silent_ota=${silent_ota} reboot_mode=${reboot_mode} androidboot.selinux=${EnableSelinux} androidboot.firstboot=${firstboot} jtag=${jtag}; "\
            "setenv bootargs ${bootargs} androidboot.hardware=amlogic androidboot.serialno=123456789ABCDEFG;"\
            "setenv bootargs ${bootargs} androidboot.slot_suffix=${active_slot};"\
            "setenv bootargs ${bootargs} androidboot.rpmb_state=${rpmb_state};"\
            "setenv bootargs ${bootargs} androidboot.memory_type=${DDR_TYPE};"\
            "setenv bootargs ${bootargs} androidboot.hwid=${hwid};"\
            "setenv bootargs ${bootargs} androidboot.model_name=${model_name};"\
            "setenv bootargs ${bootargs} androidboot.dts=${aml_dt};"\
            "setenv bootargs ${bootargs} androidboot.UbootBuildTag=${UbootBuildTag};"\
            "\0"\
        "switch_bootmode="\
            "get_rebootmode;"\
            "if test ${reboot_mode} = factory_reset; then "\
                    "run recovery_from_flash;"\
            "else if test ${reboot_mode} = update; then "\
                    "run update;"\
            "else if test ${reboot_mode} = cold_boot; then "\
                /*"run try_auto_burn; "*/\
            "else if test ${reboot_mode} = fastboot; then "\
                "fastboot;"\
            "fi;fi;fi;fi;"\
            "\0" \
        "storeboot="\
            "hdmitx output 1080p60hz;"\
            "if imgread kernel ${boot_part} ${loadaddr}; then bootm ${loadaddr}; fi;"\
            "fastboot;"\
            "\0"\
        "factory_reset_poweroff_protect="\
            "echo wipe_data=${wipe_data}; echo wipe_cache=${wipe_cache};"\
            "if test ${wipe_data} = failed; then "\
                "run init_display; run storeargs;"\
                "if mmcinfo; then "\
                    "run recovery_from_sdcard;"\
                "fi;"\
                "if usb start 0; then "\
                    "run recovery_from_udisk;"\
                "fi;"\
                "run recovery_from_flash;"\
            "fi; "\
            "if test ${wipe_cache} = failed; then "\
                "run init_display; run storeargs;"\
                "if mmcinfo; then "\
                    "run recovery_from_sdcard;"\
                "fi;"\
                "if usb start 0; then "\
                    "run recovery_from_udisk;"\
                "fi;"\
                "run recovery_from_flash;"\
            "fi; \0" \
         "update="\
            /*first usb burning, second sdc_burn, third ext-sd autoscr/recovery, last udisk autoscr/recovery*/\
            "run fastboot_burning;"\
            "run recovery_from_flash;"\
            "\0"\
        "recovery_from_sdcard="\
            "if fatload mmc 0 ${loadaddr} aml_autoscript; then autoscr ${loadaddr}; fi;"\
            "if fatload mmc 0 ${loadaddr} recovery.img; then "\
                    "if fatload mmc 0 ${dtb_mem_addr} dtb.img; then echo sd dtb.img loaded; fi;"\
                    "wipeisb; "\
                    "bootm ${loadaddr};fi;"\
            "\0"\
        "recovery_from_udisk="\
            "if fatload usb 0 ${loadaddr} aml_autoscript; then autoscr ${loadaddr}; fi;"\
            "if fatload usb 0 ${loadaddr} recovery.img; then "\
                "if fatload usb 0 ${dtb_mem_addr} dtb.img; then echo udisk dtb.img loaded; fi;"\
                "wipeisb; "\
                "bootm ${loadaddr};fi;"\
            "\0"\
        "recovery_from_flash="\
            "setMtkBT;"\
            "setenv bootargs ${bootargs} aml_dt=${aml_dt} recovery_part={recovery_part} recovery_offset={recovery_offset};"\
            "if itest ${upgrade_step} == 3; then "\
                "if ext4load mmc 1:2 ${dtb_mem_addr} /recovery/dtb.img; then echo cache dtb.img loaded; fi;"\
                "if ext4load mmc 1:2 ${loadaddr} /recovery/recovery.img; then echo cache recovery.img loaded; wipeisb; bootm ${loadaddr}; fi;"\
            "fi;"\
            "if itest ${display_is_running} != 1; then if itest ${silent_ota} != 1; then run init_display; fi; fi; "\
            "if imgread kernel ${recovery_part} ${loadaddr} ${recovery_offset}; then wipeisb; bootm ${loadaddr}; fi;"\
            "\0"\
        "init_display="\
            "osd open;osd clear;logo_display $logo_fn;bmp scale;vout output ${outputmode};"\
            "if itest ${silent_ota} == 1; then "\
                "lcd bl off;"\
            "fi"\
            "\0"\
        "check_display="\
            "get_rebootmode;"\
            "if test ${reboot_mode} = cold_boot; then "\
                "if itest ${bypass_standby} != 1; then "\
                    "echo not init_display; "\
                "else "\
                    "run init_display; "\
                "fi; "\
            "else "\
                "run init_display; "\
            "fi; "\
            "\0"\
        "bcb_cmd="\
            "get_valid_slot;"\
            "\0"\
        "upgrade_key="\
            "if gpio input GPIOAO_3; then "\
                "echo detect upgrade key; run update;"\
            "fi;"\
            "\0"\
	"irremote_update="\
		"if irkey 2500000 0xe31cfb04 0xb748fb04; then "\
			"echo read irkey ok!; " \
		"if itest ${irkey_value} == 0xe31cfb04; then " \
			"run update;" \
		"else if itest ${irkey_value} == 0xb748fb04; then " \
			"run update;\n" \
			"fi;fi;" \
		"fi;\0"\
        "adckey_update="\
            "if saradc open 2; then "\
                "if saradc getval; then "\
                    "if itest ${saradc_val} >= 0; then "\
                        "if itest ${saradc_val} <= 40; then "\
                            "run init_display;"\
                            "run storeargs;"\
                            "run upgrade_usb;"\
                            "run recovery_from_flash;"\
                        "fi;fi;fi;"\
            "fi;\0" \
        "system_factory_off="\
            "get_rebootmode; "\
            "if test ${reboot_mode} = cold_boot || itest ${silent_ota} == 1; then "\
                "run adckey_update;"\
                "echo reboot_mode=${reboot_mode}; "\
                "if test ${reboot_mode} = cold_boot && itest ${bypass_standby} != 1; then "\
                    "cec init; "\
                    "hdmirx init 0x3241; "\
                    "systemoff; "\
                "fi;"\
            "fi;"\
            "\0"\
        "upgrade_usb="\
            "if usb start 0; then "\
                "if fatload usb 0 ${loadaddr} flash_script; then uboot_update ${loadaddr}; fi;"\
            "fi;"\
            "\0"\


#define CONFIG_PREBOOT  \
            "run factory_reset_poweroff_protect;"\
            "run upgrade_check;"\
            "run check_display;"\
            "run storeargs;"\
            "bcb boot-recovery;" \
            "run system_factory_off;"\
            "run switch_bootmode;"
#define CONFIG_BOOTCOMMAND "ddr_test_cmd 0x35 4 6 40 0 0x100000 0x4000000 0 0x080009;run storeboot"

/* #define CONFIG_ENV_IS_NOWHERE  1 */
#define CONFIG_ENV_SIZE			(64*1024)
#define CONFIG_FIT			1
#define CONFIG_OF_LIBFDT		1
#define CONFIG_OF_BOARD_SETUP		1
#define CONFIG_IDME			1
#define CONFIG_ANDROID_BOOT_IMAGE	1
#define CONFIG_ANDROID_IMG		1
#define CONFIG_SYS_BOOTM_LEN		(64<<20) /* Increase max gunzip size*/

/* cpu */
#define CONFIG_CPU_CLK			1200	/* MHz. Range: 600-1800, should be multiple of 24 */

/* ddr */
#define CONFIG_DDR_SIZE			1024	/* MB //0 means ddr size auto-detect */

#ifdef UBOOT_TARGET_PRODUCT_NAME_BURGUNDY
#define CONFIG_DDR_CLK_2L		912	/* MHz, Range: 384-1200, should be multiple of 24 */
#else
#define CONFIG_DDR_CLK_2L               912     /* MHz, Range: 384-1200, should be multiple of 24 */
#endif
#define CONFIG_DDR_CLK			1008	/* MHz, Range: 384-1200, should be multiple of 24 */
#define CONFIG_DDR4_CLK			1104	/* MHz, for boards which use different ddr chip */


#define CONFIG_NR_DRAM_BANKS		1
/* DDR type setting
 *    CONFIG_DDR_TYPE_LPDDR3   : LPDDR3
 *    CONFIG_DDR_TYPE_DDR3     : DDR3
 *    CONFIG_DDR_TYPE_DDR4     : DDR4
 *    CONFIG_DDR_TYPE_AUTO     : DDR3/DDR4 auto detect */

#define CONFIG_DDR_TYPE			CONFIG_DDR_TYPE_DDR4	/* CONFIG_DDR_TYPE_AUTO */

/* DDR channel setting, please refer hardware design.
 *    CONFIG_DDR0_RANK0        : DDR0 rank0
 *    CONFIG_DDR0_RANK01       : DDR0 rank0+1
 *    CONFIG_DDR0_16BIT        : DDR0 16bit mode
 *    CONFIG_DDR0_16BIT_2      : DDR0 16bit mode, 2ranks
 *    CONFIG_DDR_CHL_AUTO      : auto detect RANK0 / RANK0+1 */

#define CONFIG_DDR_CHANNEL_SET			CONFIG_DDR0_RANK0


/* ddr functions */
#define CONFIG_DDR_FULL_TEST			0 /* 0:disable, 1:enable. ddr full test */
#define CONFIG_CMD_DDR_D2PLL			1//0 /* 0:disable, 1:enable. d2pll cmd */
#define CONFIG_CMD_DDR_TEST			1//0 /* 0:disable, 1:enable. ddrtest cmd */
#define CONFIG_DDR_LOW_POWER			0 /* 0:disable, 1:enable. ddr clk gate for lp */
#define CONFIG_DDR_ZQ_PD			0 /* 0:disable, 1:enable. ddr zq power down */
#define CONFIG_DDR_USE_EXT_VREF			0 /* 0:disable, 1:enable. ddr use external vref */
#define CONFIG_DDR4_TIMING_TEST			0 /* 0:disable, 1:enable. ddr4 timing test function */
#define CONFIG_DDR_PLL_BYPASS			0 /* 0:disable, 1:enable. ddr pll bypass function */
#define CONFIG_DDR_FUNC_RDBI			1 /* 0:disable, 1:enable. low power consumption and upgrade stability */

#define CHIP_OLD       0
#define CHIP_TXLX      1
#define CHIP_A113      2
#define CONFIG_CHIP    CHIP_TXLX//

/* storage: emmc/nand/sd */
#define		CONFIG_STORE_COMPATIBLE		1
#define 	CONFIG_ENV_OVERWRITE
#define 	CONFIG_CMD_SAVEENV
/* fixme, need fix*/

#if (defined(CONFIG_ENV_IS_IN_AMLNAND) || defined(CONFIG_ENV_IS_IN_MMC)) && defined(CONFIG_STORE_COMPATIBLE)
#error env in amlnand/mmc already be compatible;
#endif
#define		CONFIG_AML_SD_EMMC		1
#ifdef		CONFIG_AML_SD_EMMC
	#define 	CONFIG_GENERIC_MMC	1
	#define 	CONFIG_CMD_MMC		1
	#define	CONFIG_SYS_MMC_ENV_DEV		1
	#define CONFIG_EMMC_DDR52_EN		0
	#define CONFIG_EMMC_DDR52_CLK		35000000
	#define CONFIG_EMMC_BOOT1_TOUCH_REGION    (0x200000)
#endif
#define		CONFIG_PARTITIONS		1
#define 	CONFIG_SYS_NO_FLASH		1

/*SPI*/
#define CONFIG_AMLOGIC_SPI_FLASH		1
#ifdef 		CONFIG_AMLOGIC_SPI_FLASH
#undef 		CONFIG_ENV_IS_NOWHERE
/* #define		CONFIG_SPI_BOOT 1 */
#define 	CONFIG_SPI_FLASH_ATMEL
#define 	CONFIG_SPI_FLASH_EON
#define 	CONFIG_SPI_FLASH_MACRONIX
#define 	CONFIG_SPI_FLASH_SPANSION
#define 	CONFIG_SPI_FLASH_SST
#define 	CONFIG_SPI_FLASH_STMICRO
#define 	CONFIG_SPI_FLASH_WINBOND
#define		CONFIG_SPI_FRAM_RAMTRON
#define		CONFIG_SPI_M95XXX
/* #define		CONFIG_SPI_FLASH_GIGADEVICE */
/* #define		CONFIG_SPI_FLASH_PMDEVICE */
/* #define		CONFIG_SPI_NOR_SECURE_STORAGE */
#define		CONFIG_SPI_FLASH_ESMT
#define		CONFIG_SPI_FLASH		1
#define 	CONFIG_CMD_SF			1
#ifdef CONFIG_SPI_BOOT
	#define CONFIG_ENV_OVERWRITE
	#define CONFIG_ENV_IS_IN_SPI_FLASH
	#define CONFIG_CMD_SAVEENV
	#define CONFIG_ENV_SECT_SIZE		0x10000
	#define CONFIG_ENV_OFFSET		0x1f0000
#endif
#endif


/* vpu */
#define CONFIG_AML_VPU			1

/* DISPLAY & HDMITX */
/* #define CONFIG_AML_HDMITX20 1 */
#define CONFIG_AML_CANVAS		1
#define CONFIG_AML_VOUT			1
#define CONFIG_AML_OSD			1
#define CONFIG_OSD_SCALE_ENABLE		1
#define CONFIG_CMD_BMP			1

#define CONFIG_FASTBOOT_MAX_DOWNLOAD_SIZE	0x8000000

#if defined(CONFIG_AML_VOUT)
/* #define CONFIG_AML_CVBS 1 */
#endif

#define CONFIG_AML_LCD			1
#define CONFIG_AML_LCD_TV		1
#define CONFIG_AML_LCD_TABLET		1

/*
 * USB
 * Enable CONFIG_MUSB_HCD for Host functionalities MSC, keyboard
 * Enable CONFIG_MUSB_UDD for Device functionalities.
 */
/* #define CONFIG_MUSB_UDC			1 */
#define CONFIG_CMD_USB				1
#if defined(CONFIG_CMD_USB)
	#define CONFIG_GXL_XHCI_BASE            0xff500000
	#define CONFIG_GXL_USB_PHY2_BASE        0xffe09000
	#define CONFIG_GXL_USB_PHY3_BASE        0xffe09080
	#define CONFIG_USB_STORAGE		1
	#define CONFIG_USB_XHCI			1
	#define CONFIG_USB_XHCI_AMLOGIC_GXL	1
#endif /* #if defined(CONFIG_CMD_USB) */

#define CONFIG_TXLX_USB				1
/* UBOOT fastboot config */
#define CONFIG_CMD_FASTBOOT			1
#define CONFIG_FASTBOOT_FLASH_MMC_DEV		1
#define CONFIG_FASTBOOT_FLASH			1
#define CONFIG_USB_GADGET			1
#define CONFIG_USBDOWNLOAD_GADGET		1
#define CONFIG_SYS_CACHELINE_SIZE		64
#define CONFIG_FASTBOOT_MAX_DOWN_SIZE		0x8000000

#if  defined UBOOT_TARGET_PRODUCT_NAME_ANJALI
#define CONFIG_DEVICE_PRODUCT			"ANJALI"
#elif defined UBOOT_TARGET_PRODUCT_NAME_abc123
#define CONFIG_DEVICE_PRODUCT			"abc123"
#elif defined UBOOT_TARGET_PRODUCT_NAME_BURGUNDY
#define CONFIG_DEVICE_PRODUCT                   "BURGUNDY"
#define CONFIG_CMD_WOL_POWER 1
#elif defined UBOOT_TARGET_PRODUCT_NAME_RANCHO
#define CONFIG_DEVICE_PRODUCT                   "RANCHO"
#define CONFIG_CMD_WOL_POWER 1
#else
#define CONFIG_DEVICE_PRODUCT			"BLANCHE"
#endif
/* UBOOT Facotry usb/sdcard burning config */
#define CONFIG_AML_V2_FACTORY_BURN		1	/* support facotry usb burning */
#define CONFIG_AML_FACTORY_BURN_LOCAL_UPGRADE	1	/* support factory sdcard burning */
#define CONFIG_POWER_KEY_NOT_SUPPORTED_FOR_BURN	1	/* There isn't power-key for factory sdcard burning */
#define CONFIG_SD_BURNING_SUPPORT_UI		1	/* Displaying upgrading progress bar when sdcard/udisk burning */

#define CONFIG_AML_SECURITY_KEY			1
#ifndef DTB_BIND_KERNEL
#define CONFIG_UNIFY_KEY_MANAGE			1
#endif

/* net */
#define CONFIG_CMD_NET				1
#if defined(CONFIG_CMD_NET)
	#define CONFIG_DESIGNWARE_ETH		1
	#define CONFIG_PHYLIB			1
	#define CONFIG_NET_MULTI		1
	#define CONFIG_CMD_PING			1
	#define CONFIG_CMD_DHCP			1
	#define CONFIG_CMD_RARP			1
	#define CONFIG_HOSTNAME			arm_gxbb
	/* #define CONFIG_RANDOM_ETHADDR	1 */			/* use random eth addr, or default */
	#define CONFIG_ETHADDR			00:15:18:01:81:31	/* Ethernet address */
	#define CONFIG_IPADDR			10.18.9.97		/* Our ip address */
	#define CONFIG_GATEWAYIP		10.18.9.1		/* Our getway ip address */
	#define CONFIG_SERVERIP			10.18.9.113		/* Tftp server ip address */
	#define CONFIG_NETMASK			255.255.255.0
#endif /* (CONFIG_CMD_NET) */

/* other devices */
#define CONFIG_EFUSE				1
#define CONFIG_SYS_I2C_AML			1
#define CONFIG_SYS_I2C_SPEED			400000

/* commands */
#define CONFIG_CMD_CACHE			1
#define CONFIG_CMD_BOOTI			1
#define CONFIG_CMD_EFUSE			1
#define CONFIG_CMD_I2C				1
#define CONFIG_CMD_MEMORY			1
#define CONFIG_CMD_FAT				1
#define CONFIG_CMD_GPIO				1
#define CONFIG_CMD_RUN
#define CONFIG_CMD_REBOOT			1
#define CONFIG_CMD_ECHO				1
#define CONFIG_CMD_JTAG				1
#define CONFIG_CMD_AUTOSCRIPT			1
#define CONFIG_CMD_MISC				1

/*file system*/
#define CONFIG_DOS_PARTITION			1
#define CONFIG_AML_PARTITION			1
#define CONFIG_MMC				1
#define CONFIG_FS_FAT				1
#define CONFIG_FS_EXT4				1
#define CONFIG_LZO				1

/* Cache Definitions */
/* #define CONFIG_SYS_DCACHE_OFF */
/* #define CONFIG_SYS_ICACHE_OFF */

/* other functions */
#define CONFIG_NEED_BL301			1
#define CONFIG_NEED_BL32			1
#define CONFIG_CMD_RSVMEM			1
#define CONFIG_FIP_IMG_SUPPORT			1
#define CONFIG_BOOTDELAY			1
#define CONFIG_SYS_LONGHELP			1
#define CONFIG_CMD_MISC				1
#define CONFIG_CMD_ITEST			1
#define CONFIG_CMD_CPU_TEMP			1
#define CONFIG_CMD_CEC      1
#define CONFIG_CMD_HDMIRX   1
#define CONFIG_SYS_MEM_TOP_HIDE			0x08000000 /* hide 128MB for kernel reserve */

#define CONFIG_MULTI_DTB			1
#define DTB_BIND_KERNEL
#define CONFIG_PTBL_MBR				1
#define CONFIG_USE_BOOTIMAGE_DTB
#define CONFIG_BOARD_FIP			1
#define CONFIG_CMD_CHIPID			1

/* debug mode defines */
/* #define CONFIG_DEBUG_MODE			1 */
#ifdef CONFIG_DEBUG_MODE
#define CONFIG_DDR_CLK_DEBUG			636
#define CONFIG_CPU_CLK_DEBUG			600
#endif


/* support secure boot */
/* only support normal boot*/
//#define CONFIG_AML_SECURE_UBOOT			1

#if defined(CONFIG_AML_SECURE_UBOOT)

/*
 *for SRAM size limitation just disable NAND
 *as the socket board default has no NAND
 *#undef CONFIG_AML_NAND
 */

/* unify build for generate encrypted bootloader "u-boot.bin.encrypt" */
#define CONFIG_AML_CRYPTO_UBOOT			1

/*
 *unify build for generate encrypted kernel image
 *SRC : "board/amlogic/(board)/boot.img"
 *DST : "fip/boot.img.encrypt"
 */
/* #define CONFIG_AML_CRYPTO_IMG		1 */

#endif //CONFIG_AML_SECURE_UBOOT

#define CONFIG_SECURE_STORAGE			1

/* build with uboot auto test */
/* #define CONFIG_AML_UBOOT_AUTO_TEST		1 */

/* board customer ID */
/* #define CONFIG_CUSTOMER_ID  (0x6472616F624C4D41) */

#if defined(CONFIG_CUSTOMER_ID)
  #undef CONFIG_AML_CUSTOMER_ID
  #define CONFIG_AML_CUSTOMER_ID  CONFIG_CUSTOMER_ID
#endif
#define ETHERNET_INTERNAL_PHY

#define DEFAULT_MODEL_INI "HVT1_HD_33_T_1"
#define DEFAULT_MODEL_INI_PATH "/tvconfig/model/HVT1_HD_33_T_1.ini"

#define HVT1_BOARD_ID_TYPO "40001100000017"
#define HVT1_BOARD_ID "0040001100000017"
#define HVT2_BOARD_ID "0040001110100017"
#define EVT_BOARD_ID  "0040001200000017"
#define DVT_BOARD_ID  "0040001300000017"
#define PVT_BOARD_ID  "0040001400000017"

#define HVT_BOARD_ID_B  "0041000100020018"
#define EVT_BOARD_ID_B  "0041000200020018"
#define DVT_BOARD_ID_B  "0041000300020018"
#define PVT_BOARD_ID_B  "0041000400020018"

#define HVT1_BOARD_ID_TYPE 1
#define HVT2_BOARD_ID_TYPE 2
#define EVT_BOARD_ID_TYPE  3
#define DVT_BOARD_ID_TYPE  4
#define PVT_BOARD_ID_TYPE  5
#ifdef UBOOT_TARGET_PRODUCT_NAME_ANJALI
#define HVT1_L2_HWID_TYPE  8     /*for 2 layer board*/
#define HVT1_L4_HWID_TYPE  14     /*for 4 layer board*/
#define ANJALI_PRIME_HWID_TYPE  9     /*for anjali prime day board*/
#endif

#if defined(UBOOT_TARGET_PRODUCT_NAME_BURGUNDY) || defined(UBOOT_TARGET_PRODUCT_NAME_RANCHO)
#define HVT1_L2_HWID_TYPE_BURGUNDY  14     /*for BURGUNDY 2 layer board HWID:1110*/
#define HVT1_L2_HWID_TYPE_RANCHO  7     /*for RANCHO 2 layer board HWID:0111*/
#endif

/* define IDME dev_flags bits used in uboot here */
#define DEV_FLAGS_USB_NUM_VAL			2
#define DEV_FLAGS_BYPASS_SECONDARY_BOOT		4
#define DEV_FLAGS_SELINUX_FORCE_ENFORCING	32
#define DEV_FLAGS_SELINUX_FORCE_PERMISSIVE	64

/* define IDME usr_flags bits used in uboot here */
#define USR_FLAGS_STOREDEMO_MODE		4

#define CONFIG_BLANCHE				1
#define CONFIG_AMZN_FDT_FIXUP			1

/* USB port for MT7668. */
#define BT_USB_PORT_NUM	1


#ifdef CONFIG_AML_BL33_COMPRESS_ENABLE
#undef CONFIG_AML_BL33_COMPRESS_ENABLE
#endif //this project not support LZ4 compress

#define CONFIG_AML_SECURE_BOOT_V3 1

#define CONFIG_CMD_SECFLASH	1
#endif


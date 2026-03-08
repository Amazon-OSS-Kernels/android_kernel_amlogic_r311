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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __AMAZON_CUST_CONFIG_H__
#define __AMAZON_CUST_CONFIG_H__


#define CONFIG_SYS_VSNPRINTF 1


/* support uboot usb update*/
#define CONFIG_UBOOT_USB_UPDATE



//--------------please add amzn  configuration below
//add for idme
#define CONFIG_OF_BOARD_SETUP 1
#define CONFIG_IDME 1
#define CONFIG_HARDWARE_ID 1
#define CONFIG_UBOOT_LOGGER
#define CONFIG_AOCPU_LOG


 /* define IDME dev_flags bits used in uboot here */
#define DEV_FLAGS_BYPASS_SECONDARY_BOOT		4
#define DEV_FLAGS_SELINUX_FORCE_ENFORCING	32
#define DEV_FLAGS_SELINUX_FORCE_PERMISSIVE	64

#define DEV_FLAGS_USB_DEVICE	4096
#define DEV_FLAGS_ENABLE_FACTORY_TEST 2048



/*BOARD config*/
/* AMAZON Board*/
#define REF_BOARD_ID        "01A30000000A0021"
#define AMA_REF_BOARD_ID    "01A30000100A0021"  // Amazon reference square board
#define AMA_L4_BOARD_ID     "01A30000200A0021"  // Amazon reference triangle board - 4 layer
#define AMA_L2_BOARD_ID     "01A30000300A0021"  // Amazon reference triangle board - 2 layer

/* AMLOGIC Board*/
#define PROTO_BOARD_ID      "01A30000400A0021"  // Amlogic Proto (depend on ODM)
#define HVT_BOARD_ID        "01A30001000A0021"
#define HVT2_BOARD_ID       "01A30011000A0021"
#define EVT_BOARD_ID        "01A30012000A0021"
#define DVT_BOARD_ID        "01A30013000A0021"
#define PVT_BOARD_ID        "01A30014000A0021"

#define REF_BOARD_ID_TYPE 0
#define HVT_BOARD_ID_TYPE 1
#define EVT_BOARD_ID_TYPE 2
#define DVT_BOARD_ID_TYPE 3
#define PVT_BOARD_ID_TYPE 4

#define AMA_REF_BOARD_ID_TYPE    100
#define AMA_L4_BOARD_ID_TYPE     101
#define AMA_L2_BOARD_ID_TYPE     102
#define PROTO_BOARD_ID_TYPE      103



/* define IDME usr_flags bits used in uboot here */
#define USR_FLAGS_STOREDEMO_MODE		4
#define USR_FLAGS_SAVE_BL33_LOG 16

#define UBOOT_DM_VERITY_ENABLE


#define DTB_BIND_KERNEL
#define CONFIG_PTBL_MBR                1
#define CONFIG_USE_BOOTIMAGE_DTB

#define CONFIG_AMZN_FDT_FIXUP			1

// AMAZON Boardinfo
#define CONFIG_VENDOR_BOARDINFO 1

#endif

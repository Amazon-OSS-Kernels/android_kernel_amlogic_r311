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

#include <common.h>
#include <asm/arch/bl31_apis.h>
#include "anti-rollback.h"

#define FUNCID_ANTIROLLBACK_VERSION_CHECK     0xb2000010

#define FUNCID_AVB_VERSION_SET                0xb2000011
#define FUNCID_AVB_VERSION_GET                0xb2000012
#define FUNCID_AVB_LOCK_STATE_GET             0xb2000013
#define FUNCID_AVB_LOCK                       0xb2000014
#define FUNCID_AVB_UNLOCK                     0xb2000015

#define IMAGE_VERSION_TYPE_BOOTLOADER         0x001
#define IMAGE_VERSION_TYPE_RECOVERY           0x002
#define IMAGE_VERSION_TYPE_BOOT               0x003

#define KERNEL_TYPE_UNKNOWN                   0x00
#define KERNEL_TYPE_BOOT                      0x01
#define KERNEL_TYPE_RECOVERY                  0x02

static uint32_t antirollback_image_version_check(uint32_t type,
							uint32_t version)
{
	register uint32_t x0 asm("x0") = FUNCID_ANTIROLLBACK_VERSION_CHECK;
	register uint32_t x1 asm("x1") = type;
	register uint32_t x2 asm("x2") = version;

	do {
		asm volatile(
			__asmeq("%0", "x0")
			__asmeq("%1", "x0")
			__asmeq("%2", "x1")
			__asmeq("%3", "x2")
			"smc	#0\n"
			: "=r"(x0)
			: "r"(x0), "r"(x1), "r"(x2));
	} while (0);

	return x0;
}

bool check_antirollback(uint32_t kernel_version)
{
	bool ret = true;
	uint32_t type = (kernel_version >> 24);
	uint32_t version = ((kernel_version << 8) >> 8);
	if (KERNEL_TYPE_BOOT == type) {
		if (antirollback_image_version_check(IMAGE_VERSION_TYPE_BOOT,
							version) != 0) {
			printf("checking boot.img version failed\n");
			ret = false;
		}
	}
	else if (KERNEL_TYPE_RECOVERY == type) {
		if (antirollback_image_version_check(
					IMAGE_VERSION_TYPE_RECOVERY,
					version) != 0) {
			printf("checking recovery.img version failed\n");
			ret = false;
		}
	}
	else {
		printf("the kernel type is unknown\n");
		ret = false;
	}

	if (ret)
		printf("checking version success\n");

	return ret;
}

bool set_avb_antirollback(uint32_t index, uint32_t version)
{
	register uint32_t x0 asm("x0") = FUNCID_AVB_VERSION_SET;
	register uint32_t x1 asm("x1") = index;
	register uint32_t x2 asm("x2") = version;

	do {
		asm volatile(
			__asmeq("%0", "x0")
			__asmeq("%1", "x0")
			__asmeq("%2", "x1")
			__asmeq("%3", "x2")
			"smc	#0\n"
			: "=r"(x0)
			: "r"(x0), "r"(x1), "r"(x2));
	} while (0);

	return 0 == x0;
}

bool get_avb_antirollback(uint32_t index, uint32_t* version)
{
	register uint32_t x0 asm("x0") = FUNCID_AVB_VERSION_GET;
	register uint32_t x1 asm("x1") = index;

	do {
		asm volatile(
			__asmeq("%0", "x0")
			__asmeq("%1", "x1")
			__asmeq("%2", "x0")
			__asmeq("%3", "x1")
			"smc	#0\n"
			: "=r"(x0), "=r"(x1)
			: "r"(x0), "r"(x1));
	} while (0);

	if (0 == x0)
		*version = x1;

	return 0 == x0;
}

bool get_avb_lock_state(uint32_t* lock_state)
{
	register uint32_t x0 asm("x0") = FUNCID_AVB_LOCK_STATE_GET;
	register uint32_t x1 asm("x1") = 0;

	do {
		asm volatile(
			__asmeq("%0", "x0")
			__asmeq("%1", "x1")
			__asmeq("%2", "x0")
			"smc	#0\n"
			: "=r"(x0), "=r"(x1)
			: "r"(x0));
	} while (0);

	if (0 == x0)
		*lock_state = x1;

	return 0 == x0;
}

bool avb_lock(void)
{
	register uint32_t x0 asm("x0") = FUNCID_AVB_LOCK;

	do {
		asm volatile(
			__asmeq("%0", "x0")
			__asmeq("%1", "x0")
			"smc	#0\n"
			: "=r"(x0)
			: "r"(x0));
	} while (0);

	return 0 == x0;
}

bool avb_unlock(void)
{
	register uint32_t x0 asm("x0") = FUNCID_AVB_UNLOCK;

	do {
		asm volatile(
			__asmeq("%0", "x0")
			__asmeq("%1", "x0")
			"smc	#0\n"
			: "=r"(x0)
			: "r"(x0));
	} while (0);

	return 0 == x0;
}

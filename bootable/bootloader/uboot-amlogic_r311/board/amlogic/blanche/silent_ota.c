/*
 * secure_boot.c
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 */

#include <asm/arch/secure_apb.h>
#include <asm/io.h>
#include <asm/types.h>

#include "amzn_silent_ota.h"

u32 is_silent_ota()
{
	u32 status, ret = 0;

	status = *((volatile u32 *)PREG_STICKY_REG0);
	if (status & SPARSE_SPECIAL_MODE_SILENT_OTA)
		ret = 1;

	return ret;
}

void set_silent_ota_flag(void)
{
	u32 status;

	status = *((volatile u32 *)PREG_STICKY_REG0);
	*((volatile u32 *)PREG_STICKY_REG0) = (u32)(status | SPARSE_SPECIAL_MODE_SILENT_OTA);
}

void clear_silent_ota_flag(void)
{
	u32 status;

	status = *((volatile u32 *)PREG_STICKY_REG0);
	*((volatile u32 *)PREG_STICKY_REG0) = (u32)(status & ~SPARSE_SPECIAL_MODE_SILENT_OTA);
}



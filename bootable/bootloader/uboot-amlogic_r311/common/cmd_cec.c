/*
 * common/cmd_cec.c
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

#include <common.h>
#include <command.h>
#include <asm/cpu_id.h>
#include <asm/arch/io.h>
#include <asm/arch/clock.h>
#include <amlogic/aml_cec.h>

#define CEC_VERSION "Ver 2017/11/14\n"

static void cec_init(int logic_addr)
{
	printf("%s :%d\n", __func__, logic_addr);
	cec_hw_init(logic_addr);
}

static int do_cec(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int logic_addr;

	printf(CEC_VERSION);

	if (argc ==  2)
		logic_addr = simple_strtoul(argv[1], NULL, 10);
	else
		logic_addr = 0;

	cec_init(logic_addr);

	return 0;
}

U_BOOT_CMD(
		cec, 2, 1, do_cec,
		"Amlogic cec",
		"	- hdmi cec function \n"
		"	- param: logic addr \n"
		"\n"
);



// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * common/cmd_aocpu_log.c
 *
 * Copyright (C) 2021 Amlogic, Inc. All rights reserved.
 *
 */
#include <config.h>
#include <common.h>
#include <asm/arch/io.h>
#include <command.h>
#include <malloc.h>

extern void aocpu_log_external_print(void);

int do_log_output(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 2)
		return CMD_RET_USAGE;
	if (strncmp(argv[1], "aocpu", 5) == 0)
		aocpu_log_external_print();

	return 0;
}

static char log_output_help_text[] =
	"[output amp cpu log, [aocpu]]\n"
	"  [aocpu] output aocpu log\n"
	"          -example: [log_output aocpu];\n"
	;

U_BOOT_CMD(
	log_output,	5,	1,	do_log_output,
	"log_output commands", log_output_help_text
);

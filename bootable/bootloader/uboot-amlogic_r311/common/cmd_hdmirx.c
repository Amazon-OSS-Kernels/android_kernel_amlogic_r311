 /*
  * common/cmd_hdmirx.c
  *
  * Copyright (C) 2012 AMLOGIC, INC. All Rights Reserved.
  * Author: hongmin hua <hongmin hua@amlogic.com>
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the smems of the GNU General Public License as published by
  * the Free Software Foundation; version 2 of the License.
  */

#include <common.h>
#include <command.h>
#include <asm/cpu_id.h>
#include <asm/arch/io.h>
#include <amlogic/aml_hdmirx.h>

#define HDMIRX_VERSION "Ver 2017/11/14\n"

static int hdmirx_hpd(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	if (argc < 1)
		return cmd_usage(cmdtp);

	return CMD_RET_SUCCESS;
}

static int hdmirx_edid(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	if (argc < 1)
		return cmd_usage(cmdtp);

	return CMD_RET_SUCCESS;
}

static int hdmirx_init(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	unsigned int port_map;

	printf(HDMIRX_VERSION);
	if (argc < 1)
		return cmd_usage(cmdtp);

	if (argc == 2) {
		port_map = simple_strtoul(argv[1], NULL, 16);
		hdmirx_hw_init(port_map);
		return CMD_RET_SUCCESS;
	} else {
		printf("HDMIRX: Error! No port map param\n");
		return CMD_RET_FAILURE;
	}
}

static cmd_tbl_t cmd_hdmi_sub[] = {
	U_BOOT_CMD_MKENT(init, 1, 1, hdmirx_init, "", ""),
	U_BOOT_CMD_MKENT(hpd, 1, 1, hdmirx_hpd, "", ""),
	U_BOOT_CMD_MKENT(edid, 3, 1, hdmirx_edid, "", ""),
};

static int do_hdmirx(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	cmd_tbl_t *c;

	if (argc < 2)
		return cmd_usage(cmdtp);

	argc--;
	argv++;

	c = find_cmd_tbl(argv[0], &cmd_hdmi_sub[0], ARRAY_SIZE(cmd_hdmi_sub));

	if (c)
		return  c->cmd(cmdtp, flag, argc, argv);
	else
		return cmd_usage(cmdtp);
}

U_BOOT_CMD(hdmirx, CONFIG_SYS_MAXARGS, 0, do_hdmirx,
	   "hdmirx init function\n",
				"hdmirx init\n"
				"      param: port_map\n"
				"\n"
);


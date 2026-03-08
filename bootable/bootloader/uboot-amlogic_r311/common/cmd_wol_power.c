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

#include <config.h>
#include <common.h>
#include <asm/arch/io.h>
#include <command.h>
#include <malloc.h>
#include <asm/arch/mailbox.h>

static void set_wol_power_cmdline(int enable) {

	char *pARG = getenv("bootargs");
	if (pARG == NULL)
	{
		printf("Can't get bootargs\n");
		return;
	}
	//printf("1 bootargs=%s\n",pARG);
	char *szBuffer=malloc(strlen(pARG)+64);
	if (szBuffer == NULL)
	{
		printf("aml log : internal sys error!\n");
		return;
	}
	memset(szBuffer+strlen(pARG),0,64);
	strcpy(szBuffer,pARG);
	char *pFind = strstr(szBuffer,"wol_power=");
	if (!pFind)
		sprintf(szBuffer,"%s wol_power=%d",pARG, enable);
	else
		pFind[23] = enable ? '1':'0';

	//printf("2 bootargs=%s\n",szBuffer);

	setenv("bootargs",szBuffer);
	free(szBuffer);
	szBuffer = 0;
}
static int do_wol_power_enable(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	/*Enable Wol Power when the system suspend.*/
	set_WOL_power(1);
	set_wol_power_cmdline(1);
	return 0;
}

static int do_wol_power_disable(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	/*Disable Wol Power when the system suspend.*/
	set_WOL_power(0);
	set_wol_power_cmdline(0);
	return 0;
}


static int do_wol_power_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i = 0;
	for (i =0; i<10000;i++) {
		set_WOL_power(i);
		udelay(100000);
	}
	return 0;
}

static int do_wol_power_get(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned int value = 0;

	if (get_WOL_power(&value) != 0) {
		printf("get WOL power config error!\n");
		return 1;
	}
	printf("WOL power status:%d\n", value);
	return 0;
}

static cmd_tbl_t cmd_wol_power_sub[] = {
	U_BOOT_CMD_MKENT(enable,  2, 0, do_wol_power_enable, "", ""),
	U_BOOT_CMD_MKENT(disable, 2, 0, do_wol_power_disable, "", ""),
	U_BOOT_CMD_MKENT(test,   4, 0, do_wol_power_test, "", ""),
	U_BOOT_CMD_MKENT(get,   2, 0, do_wol_power_get, "", ""),
};


static int do_wol_power(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	cmd_tbl_t *c;

	argc--;
	argv++;
	c = find_cmd_tbl(argv[0], &cmd_wol_power_sub[0], ARRAY_SIZE(cmd_wol_power_sub));
	if (c) {
		return c->cmd(cmdtp, flag, argc, argv);
	} else {
		cmd_usage(cmdtp);
		return 1;
	}
}





U_BOOT_CMD(
	wol_power,	4,	1,	do_wol_power,
	"wol_power setting",
	"enable      --enable wol power when str\n"
	"wol_power disable     --disable wol power when str\n"
);

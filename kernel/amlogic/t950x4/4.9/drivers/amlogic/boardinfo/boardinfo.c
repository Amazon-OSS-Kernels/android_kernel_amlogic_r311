/*
 * drivers/amlogic/boardinfo/boardinfo.c
 *
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
 */

#include <linux/device.h>

#define OWNER_NAME "boardinfo"


/*
 * the boardinfo map to AMAZON board id.
 * the map be defined in
 * bl33\board\amlogic\abc123\boardinfo.c
 * boardinfo value:
 * REF_BOARD_ID_TYPE 0
 * HVT_BOARD_ID_TYPE 1
 * EVT_BOARD_ID_TYPE 2
 * DVT_BOARD_ID_TYPE 3
 * PVT_BOARD_ID_TYPE 4
 * AMA_REF_BOARD_ID_TYPE    100
 * AMA_L4_BOARD_ID_TYPE     101
 * AMA_L2_BOARD_ID_TYPE     102
 * PROTO_BOARD_ID_TYPE      103
 */


char boardinfo[10] = "";

static int __init boardinfo_para_setup(char *str)
{
	if (str != NULL)
		sprintf(boardinfo, "%s", str);

	pr_info("boardinfo: %s\n", boardinfo);
	return 0;
}

__setup("androidboot.boardinfo=", boardinfo_para_setup);

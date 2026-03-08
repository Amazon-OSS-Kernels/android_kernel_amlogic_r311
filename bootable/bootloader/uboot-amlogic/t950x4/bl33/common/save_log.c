// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <stdarg.h>
#include <iomux.h>
#include <malloc.h>
#include <serial.h>
#include <environment.h>
#include <amlogic/save_log.h>

//add amzn start
#ifdef CONFIG_IDME
#include <idme.h>
#endif


DECLARE_GLOBAL_DATA_PTR;
/*
 * we split a 1mb block to 16*64K part, each part with a magic and a idx
 * other reset are logs.
 */
#define LOG_BUFFER_SIZE		(64 * 1024)
#define LOG_BUFFER_COUNT	(128)
#define LOG_MAGIC		(0x474f4c55)	/* ULOG */

#define LOG_PARTITION_NAME	"bak"

#define CONFIG_BL2_LOG
#ifdef CONFIG_BL2_LOG
#define CONFIG_BL2_LOG_ADDR	0x8000010
#define BL2_LOG_SIZE_ADDR	0x8000000
#define BL2_LOG_SIZE		0xA00
#endif


int can_save = 0;
static int curr_log_idx = -1;
static int log_offset = 0;
static bool enable_save_bl33_log = false;

struct bl33_log {
	unsigned int magic;
	unsigned int log_count;
	char buffer[LOG_BUFFER_SIZE - sizeof(int) * 2];
};
static struct bl33_log *log = NULL;

static int pre_log_overflow = 0;

static void bl33_log_pre_save(const char *s)
{
	int len;
	char *pre_log_addr = (char *)CONFIG_PRE_BL33_LOG_ADDR;

	len = strlen(s);
	if (len + gd->pre_log_size >= PRE_LOG_SIZE) {
		/* ignore when overflow */
		pre_log_overflow = 1;
		return;
	}
	strcpy(pre_log_addr + gd->pre_log_size, s);
	gd->pre_log_size += len;
}

#if defined(CONFIG_IDME)
static void check_save_bl33_log(void)
{
    char usr_flags_buf[8] = {0};
    unsigned usr_flags = 0;

    /* treat usr_flags as an unsigned integer in hex */
    if (!idme_get_var_external("usr_flags", usr_flags_buf, sizeof(usr_flags_buf) - 1)) {
        usr_flags = simple_strtoul(usr_flags_buf, NULL, 16);
        if (usr_flags & USR_FLAGS_SAVE_BL33_LOG) {
			enable_save_bl33_log = true;
			printf("%s, enable to save bl33 log into emmc\n",__func__);
			return;
        }
    }

	printf("%s, did not save bl33 log into emmc\n",__func__);
	return;
}
#endif

#ifdef CONFIG_BL2_LOG
void save_bl2_log()
{
	char *bl2_log = (char *)CONFIG_BL2_LOG_ADDR;
	uint32_t bl2_log_size = *( (uint32_t *)BL2_LOG_SIZE_ADDR);

	if (bl2_log_size >= BL2_LOG_SIZE)
		bl2_log_size = BL2_LOG_SIZE -1 ;

	bl2_log[bl2_log_size] = '\0';
	save_bl33_log(bl2_log);
}
#endif

int bl33_log_init(void)
{
	int i, ret;
	unsigned int max_cnt = 0, idxidx = 0;
	unsigned int *buf;
	char *pre_log_addr = (char *)CONFIG_PRE_BL33_LOG_ADDR;

#if defined(CONFIG_IDME)
	check_save_bl33_log();
#endif
	if (false == enable_save_bl33_log){
		printf("%s bl33 log save disable!!\n", __func__);
		return 0;
	}

	printf("%s, pre_log_overflow:%d\n", __func__, pre_log_overflow);
	log = malloc(sizeof(*log));
	if (!log) {
		printf("alloc buffer failed\n");
		return -1;
	}
	buf = (unsigned int *)log->buffer;
	for (i = 0; i < LOG_BUFFER_COUNT; i++) {
		memset(buf, 0, 8);
		ret = aml_mmc_read(LOG_PARTITION_NAME,
				   i * LOG_BUFFER_SIZE,
				   8,
				   buf);
		if (ret < 0) {
			printf("%s read fail, no log will be written!!!\n", __func__);
			return 0;
		}

		if (buf[0] != LOG_MAGIC) {
			/* This is not a valid log part */
			printf("%s, bad log magic:%08x, ret:%d\n",
				__func__, buf[0], ret);
			break;
		}
		if (buf[1] > max_cnt) {
			max_cnt = buf[1];
			idxidx  = i;
		}
	}
	if (max_cnt) {	/* find max log_count */
		printf("%s, max cnt:%d, idx:%d\n",
			__func__, max_cnt, idxidx);
		curr_log_idx = idxidx + 1;
		if (curr_log_idx >= LOG_BUFFER_COUNT) /* wrap back */
			curr_log_idx = 0;
	} else {
		curr_log_idx = 0;
	}
	memset(log, 0, sizeof(*log));

	log->magic = LOG_MAGIC;
	log->log_count = max_cnt + 1;
	if (gd->pre_log_size) {	/* pad pre-log */
		memcpy(log->buffer, pre_log_addr, gd->pre_log_size);
		log_offset = 8 + gd->pre_log_size;
	} else
		log_offset = 8;
	ret = aml_mmc_write(LOG_PARTITION_NAME,
			    curr_log_idx * LOG_BUFFER_SIZE,
			    sizeof(*log),
			    log);
	can_save = 1;
	printf("%s, select block %d, off:%d, pre_log_addr:%p, ret:%d\n",
		__func__, curr_log_idx, log_offset, pre_log_addr, ret);
#ifdef CONFIG_BL2_LOG
	save_bl2_log();
#endif
	return 0;
}

void save_bl33_log(const char *buf)
{
	int len, bl_len, ret;
	int old_off = log_offset;
	int start_blk, end_blk;
	unsigned char *wp;

	if (false == enable_save_bl33_log){
		return;
	}

	if (curr_log_idx < 0) {
		bl33_log_pre_save(buf);
		return;
	}

	if (!can_save)
		return;

	bl_len = get_log_block_size();
	len    = strlen(buf);
	if (log_offset + len >= LOG_BUFFER_SIZE) {
		return;
	}
	strcpy(log->buffer + log_offset - 8, buf);
	log_offset += len;
	start_blk = old_off / bl_len;
	end_blk   = log_offset / bl_len;
	do {	/* update block */
		wp = (unsigned char *)log;
		wp = wp + start_blk * bl_len;
		ret = aml_mmc_write(LOG_PARTITION_NAME,
				    curr_log_idx * LOG_BUFFER_SIZE + start_blk * bl_len,
				    bl_len,
				    wp);
		if (ret < 0) {
			break;
		}
		start_blk++;
	} while (start_blk <= end_blk);
#if 0
	can_save = 0;
	printf("\n");
	print_buffer((unsigned long)log, (void *)log, 1, log_offset, 0);
	printf("\n");
	can_save = 1;
#endif
}

static void show_long_buf(struct bl33_log *tmp_log)
{
	char *p, *n;

#if 0
	print_buffer((unsigned long)tmp_log, (void *)tmp_log, 1, sizeof(*tmp_log), 0);
#endif
	p = tmp_log->buffer;
	do {
		n = strchr(p, '\n');
		if (n) {
			*n = '\0';
			printf(">>  %s\n", p);
			p = n + 1;
		}
	} while (n);
}

int dump_uboot_log(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int i, ret;
	struct bl33_log *tmp_log;

	can_save = 0;
	if (argc == 2) {
		if (!strncmp(argv[1], "-1", 2)) {
			printf("erase all saved boot log\n");
			ret = aml_log_erase(LOG_PARTITION_NAME);
			can_save = 1;
			return ret;
		}
	}
	tmp_log = malloc(sizeof(*tmp_log));
	if (!tmp_log) {
		printf("alloc buffer failed\n");
		can_save = 1;
		return -1;
	}
	for (i = 0; i < LOG_BUFFER_COUNT; i++) {
		memset(tmp_log, 0, sizeof(*tmp_log));
		ret = aml_mmc_read(LOG_PARTITION_NAME,
				   i * LOG_BUFFER_SIZE,
				   sizeof(*tmp_log),
				   tmp_log);
		if (ret)
			break;
		if (tmp_log->magic != LOG_MAGIC)
			continue;

		printf("--------part:%2d, log_cnt:%8d ---------\n", i, tmp_log->log_count);
		show_long_buf(tmp_log);
	}
	free(tmp_log);

	can_save = 1;
	return 0;
}

U_BOOT_CMD(
	bl33_log,	2,	1,	dump_uboot_log,
	"print all saved bl33 log",
	"no parameters"
);

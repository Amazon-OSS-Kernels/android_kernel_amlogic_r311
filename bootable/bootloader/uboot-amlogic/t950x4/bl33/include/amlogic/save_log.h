/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __SAVE_LOG_H__
#define __SAVE_LOG_H__

int aml_mmc_read(const char *part_name, loff_t offset, size_t len, void *buffer);
int aml_mmc_write(const char *part_name, loff_t offset, size_t len, void *buffer);
int aml_log_erase(const char *log_part_name);

void save_bl33_log(const char *msg);
int bl33_log_init(void);
int bl33_log_pre_init(void);
int get_log_block_size(void);

#endif /* __SAVE_LOG_H__ */

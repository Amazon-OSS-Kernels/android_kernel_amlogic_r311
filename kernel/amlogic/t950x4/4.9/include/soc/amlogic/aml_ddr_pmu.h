/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AML_DDR_PMU_H__
#define __AML_DDR_PMU_H__

#include <linux/compiler.h>

#define MAX_CHANNEL_NUM		8
#define MAX_CHANNEL		MAX_CHANNEL_NUM
#define PORT_MAJOR		32
#define DEFAULT_XTAL_FREQ	24000000UL

enum {
	CYCLE_COUNTER_ID,
	ALL_CHAN_COUNTER_ID,
	ALL_CHAN_IDLE_CNT_ID,
	CHAN1_COUNTER_ID,
	CHAN2_COUNTER_ID,
	CHAN3_COUNTER_ID,
	CHAN4_COUNTER_ID,
	CHAN5_COUNTER_ID,
	CHAN6_COUNTER_ID,
	CHAN7_COUNTER_ID,
	CHAN8_COUNTER_ID,
	COUNTER_MAX_ID,
};

#define EVENT_SUPPORT	1

struct dmc_hw_info;

struct ddr_grant_info {
	u64 all_grant;
	union {
		u64 all_req;
		struct {
			u64 all_chann_idle;
			u64 all_grant16;
		};
	};
	u64 channel_grant[MAX_CHANNEL_NUM];
};

struct dmc_pmu_hw_ops {
	void (*enable)(struct dmc_hw_info *info);
	void (*disable)(struct dmc_hw_info *info);
	void (*config_port)(struct dmc_hw_info *info, int port, int chann);
	int (*irq_handler)(struct dmc_hw_info *info,
			struct ddr_grant_info *dg);
	/* compatible with ddr banwidth driver */
	int (*irq_identify)(struct dmc_hw_info *info);
	void (*get_counters)(struct dmc_hw_info *info,
			struct ddr_grant_info *dg);
};

struct dmc_hw_info {
	char name[100];
	struct dmc_pmu_hw_ops *ops;
	void __iomem *ddr_reg[4];
	unsigned long timer_value; /* timer value in TIMER register */
	void __iomem *pll_reg;
	int irq_num; /* irq vector number */
	int dmc_nr;
	int chann_nr;
	int id; /* The number of supported channels */
	u8 event_flag[COUNTER_MAX_ID]; /*identify supported events */
};

struct dmc_pmu_hw_ops *t5_dmc_pmu_init(struct dmc_hw_info *info);
#endif /* __AML_DDR_PMU_H__ */

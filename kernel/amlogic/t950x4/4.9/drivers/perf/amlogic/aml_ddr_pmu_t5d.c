// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */


#include <linux/err.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <asm-generic/io.h>

#include <linux/amlogic/cpu_version.h>
#include <soc/amlogic/aml_ddr_pmu.h>

#define DMC_QOS_IRQ			BIT(30)

#define DMC_MON_CTRL0			((0x0020  << 2))
#define DMC_MON_TIMER			((0x0021  << 2))
#define DMC_MON_ALL_IDLE_CNT		((0x0022  << 2))
#define DMC_MON_ALL_BW			((0x0023  << 2))
#define DMC_MON_ALL16_BW		((0x0024  << 2))

#define DMC_MON0_CTRL			((0x0030  << 2))
#define DMC_MON0_CTRL1			((0x0031  << 2))
#define DMC_MON0_CTRL2			((0x0032  << 2))
#define DMC_MON0_BW			((0x0033  << 2))

#define DMC_MON1_CTRL			((0x0034  << 2))
#define DMC_MON1_CTRL1			((0x0035  << 2))
#define DMC_MON1_CTRL2			((0x0036  << 2))
#define DMC_MON1_BW			((0x0037  << 2))

#define DMC_MON2_CTRL			((0x0038  << 2))
#define DMC_MON2_CTRL1			((0x0039  << 2))
#define DMC_MON2_CTRL2			((0x003a  << 2))
#define DMC_MON2_BW			((0x003b  << 2))

#define DMC_MON3_CTRL			((0x003c  << 2))
#define DMC_MON3_CTRL1			((0x003d  << 2))
#define DMC_MON3_CTRL2			((0x003e  << 2))
#define DMC_MON3_BW			((0x003f  << 2))

#define DMC_MON4_CTRL			((0x00c0  << 2))
#define DMC_MON4_CTRL1			((0x00c1  << 2))
#define DMC_MON4_CTRL2			((0x00c2  << 2))
#define DMC_MON4_BW			((0x00c3  << 2))

#define DMC_MON5_CTRL			((0x00c4  << 2))
#define DMC_MON5_CTRL1			((0x00c5  << 2))
#define DMC_MON5_CTRL2			((0x00c6  << 2))
#define DMC_MON5_BW			((0x00c7  << 2))

#define DMC_MON6_CTRL			((0x00c8  << 2))
#define DMC_MON6_CTRL1			((0x00c9  << 2))
#define DMC_MON6_CTRL2			((0x00ca  << 2))
#define DMC_MON6_BW			((0x00cb  << 2))

#define DMC_MON7_CTRL			((0x00cc  << 2))
#define DMC_MON7_CTRL1			((0x00cd  << 2))
#define DMC_MON7_CTRL2			((0x00ce  << 2))
#define DMC_MON7_BW			((0x00cf  << 2))

#define DMC_VERSION			((0x004f  << 2))

static const u32 dmc_version_supported_list[] = {0x1000008};

static void t5_dmc_port_config(struct dmc_hw_info *info, int port, int channel);

static void t5_dmc_set_timer(struct dmc_hw_info *info)
{
	unsigned long clock_count = info->timer_value;

	pr_debug("ddr clock is %lu\n", clock_count);

	/* set timer trigger clock_cnt 1s*/
	writel(clock_count, info->ddr_reg[0] + DMC_MON_TIMER);
}

static void t5_dmc_counter_disable(struct dmc_hw_info *info)
{
	unsigned int val;
	int i;

	val = readl(info->ddr_reg[0] + DMC_MON_CTRL0);
	/* clear irq flags */
	writel(val, info->ddr_reg[0] + DMC_MON_CTRL0);

	/* clear port channal mapping */
	for (i = 0; i < info->chann_nr; i++)
		t5_dmc_port_config(info, -1, i);
}

static void t5_dmc_port_config(struct dmc_hw_info *info, int port, int channel)
{
	unsigned int val;
	unsigned int off = 0;
	int subport = -1;

	pr_debug("port %d, channel %d\n", port, channel);

	if (channel < 4)
		off = channel * 16 + DMC_MON0_CTRL;
	else
		off = (channel - 4) * 16 + DMC_MON4_CTRL;

	/* clear all port mask */
	if (port < 0) {
		writel(0, info->ddr_reg[0] + off + 4);	/* DMC_MON*_CTRL1 */
		writel(0, info->ddr_reg[0] + off + 8);	/* DMC_MON*_CTRL2 */
		return;
	}

	if (port >= PORT_MAJOR)
		subport = port - PORT_MAJOR;

	if (subport < 0) {
		val = readl(info->ddr_reg[0] + off + 4);
		val |=  (1 << port);
		writel(val, info->ddr_reg[0] + off + 4);	/* DMC_MON*_CTRL1 */
		val = 0xffff;
		writel(val, info->ddr_reg[0] + off + 8);	/* DMC_MON*_CTRL2 */
	} else {
		val = (0x1 << 23);	/* select device */
		writel(val, info->ddr_reg[0] + off + 4);
		val = readl(info->ddr_reg[0] + off + 8);
		val |= (1 << subport);
		writel(val, info->ddr_reg[0] + off + 8);
	}
}

static unsigned long t5_get_dmc_freq_quick(struct dmc_hw_info *info)
{
	unsigned int val;
	unsigned int n, m, od1;
	unsigned int od_div = 0xfff;
	unsigned long freq = 0;

	val = readl(info->pll_reg);
	val = val & 0xfffff;
	switch ((val >> 16) & 7) {
	case 0:
		od_div = 2;
		break;

	case 1:
		od_div = 3;
		break;

	case 2:
		od_div = 4;
		break;

	case 3:
		od_div = 6;
		break;

	case 4:
		od_div = 8;
		break;

	default:
		break;
	}

	m = val & 0x1ff;
	n = ((val >> 10) & 0x1f);
	od1 = (((val >> 19) & 0x1)) == 1 ? 2 : 1;
	freq = DEFAULT_XTAL_FREQ / 1000;	/* avoid overflow */
	if (n)
		freq = ((((freq * m) / n) >> od1) / od_div) * 1000;

	return freq;
}

static void t5_dmc_counter_enable(struct dmc_hw_info *info)
{
	unsigned int val;
	unsigned long clk = readl(info->ddr_reg[0] + DMC_MON_TIMER);

	if (clk != info->timer_value)
		writel(info->timer_value, info->ddr_reg[0] + DMC_MON_TIMER);

	/* enable all channel */
	val =  (0x01 << 31) |	/* enable bit */
	       (0x01 << 20) |	/* use timer  */
	       (0xff <<  0);
	writel(val, info->ddr_reg[0] + DMC_MON_CTRL0);
}

static void t5_dmc_get_counters(struct dmc_hw_info *info,
				struct ddr_grant_info *dg)
{
	int i;
	int off;
	/*
	 * get total bytes by each channel, each cycle 16 bytes;
	 */
	dg->all_grant    = readl(info->ddr_reg[0] + DMC_MON_ALL_BW);
	dg->all_grant16  = readl(info->ddr_reg[0] + DMC_MON_ALL16_BW);

	for (i = 0; i < info->chann_nr; i++) {
		if (i < 4)
			off = i * 16 + DMC_MON0_BW;
		else
			off = (i - 4) * 16 + DMC_MON4_BW;
		dg->channel_grant[i] = readl(info->ddr_reg[0] + off);
	}

	pr_debug("dg->all_grant %llu dg->channel_grant[0] %llu\n",
			dg->all_grant, dg->channel_grant[0]);
}

static int t5_dmc_irq_identify(struct dmc_hw_info *info)
{

	if (info->timer_value != readl(info->ddr_reg[0] + DMC_MON_TIMER))
		return 0;

	return 1;
}

static int t5_dmc_irq_handler(struct dmc_hw_info *info,
				struct ddr_grant_info *dg)
{
	unsigned int val;
	int ret = -1;

	val = readl(info->ddr_reg[0] + DMC_MON_CTRL0);
	if (val & DMC_QOS_IRQ) {
		t5_dmc_get_counters(info, dg);
		/* clear irq flags */
		writel(val, info->ddr_reg[0] + DMC_MON_CTRL0);

		ret = 0;
	}
	return ret;

}

static struct dmc_pmu_hw_ops tm2_ops = {
	.enable		= t5_dmc_counter_enable,
	.disable	= t5_dmc_counter_disable,
	.irq_handler	= t5_dmc_irq_handler,
	.irq_identify	= t5_dmc_irq_identify,
	.get_counters	= t5_dmc_get_counters,
	.config_port	= t5_dmc_port_config,
};

/* the event list which t5 supports */
static const int event_support[] = {
	CYCLE_COUNTER_ID,
	ALL_CHAN_COUNTER_ID,
	CHAN1_COUNTER_ID,
	CHAN2_COUNTER_ID,
	CHAN3_COUNTER_ID,
	CHAN4_COUNTER_ID,
	CHAN5_COUNTER_ID,
	CHAN6_COUNTER_ID,
	CHAN7_COUNTER_ID,
	CHAN8_COUNTER_ID,
};

static void t5_dmc_range_config(struct dmc_hw_info *info, int channel,
				unsigned int start, unsigned int end)
{
	unsigned int val;
	unsigned int off = 0;

	if (channel < 4)
		off = channel * 16 + DMC_MON0_CTRL;
	else
		off = (channel - 4) * 16 + DMC_MON4_CTRL;

	val = (start >> 16) | (end & 0xffff0000);
	writel(val, info->ddr_reg[0] + off);			/* DMC_MON*_CTRL */
}

struct dmc_pmu_hw_ops *t5_dmc_pmu_init(struct dmc_hw_info *info)
{
	unsigned int i;
	unsigned int version = readl(info->ddr_reg[0] + DMC_VERSION);

	pr_info("readl(info->ddr_reg[0] + DMC_VERSION) %x\n", version);

	for (i = 0; i < ARRAY_SIZE(dmc_version_supported_list); i++)
		if (dmc_version_supported_list[i] == version) {
			pr_info("find the supported dmc %x\n", version);
			break;
		}

	if (i == ARRAY_SIZE(dmc_version_supported_list))
		return ERR_PTR(-ENODEV);

	info->chann_nr = 8;

	for (i = 0; i < ARRAY_SIZE(event_support); i++)
		info->event_flag[event_support[i]] = EVENT_SUPPORT;

	info->timer_value = t5_get_dmc_freq_quick(info) / 10; //100ms

	t5_dmc_set_timer(info);

	for (i = 0; i < info->chann_nr; i++)
		t5_dmc_range_config(info, i, 0, 0xffffffff);

	t5_dmc_counter_disable(info);

	strcpy(info->name, "t5d_ddr_pmu");

	return &tm2_ops;
}

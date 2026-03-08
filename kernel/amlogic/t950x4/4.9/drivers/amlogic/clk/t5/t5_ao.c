/*
 * drivers/amlogic/clk/t5/t5_ao.c
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

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <dt-bindings/clock/amlogic,t5-aoclkc.h>
#include "../clkc.h"
#include "../clk-dualdiv.h"

#define AO_RTI_PWR_CNTL_REG0		0x10 /* 0x4 offset in datasheet1*/
#define AO_CLK_GATE0			0x4c /* 0x13 offset in datasheet1 */
#define AO_CLK_GATE0_SP			0x50 /* 0x14 offset in datasheet1 */
#define AO_SAR_CLK			0x90 /* 0x24 offset in datasheet1 */
#define AO_CECB_CLK_CNTL_REG0		(0xa0 << 2)
#define AO_CECB_CLK_CNTL_REG1		(0xa1 << 2)

#define MESON_AO_GATE_T5(_name, _reg, _bit)			\
struct clk_gate _name = {					\
	.reg = (void __iomem *) _reg,				\
	.bit_idx = (_bit),					\
	.lock = &clk_lock,					\
	.hw.init = &(struct clk_init_data) {			\
		.name = #_name,					\
		.ops = &clk_gate_ops,				\
		.parent_names = (const char *[]){ "t5_clk81" },	\
		.num_parents = 1,				\
		.flags = CLK_IGNORE_UNUSED,			\
	},							\
}

MESON_AO_GATE_T5(t5_ao_ahb_bus,		AO_CLK_GATE0, 0);
MESON_AO_GATE_T5(t5_ao_ir,		AO_CLK_GATE0, 1);
MESON_AO_GATE_T5(t5_ao_i2c_master,	AO_CLK_GATE0, 2);
MESON_AO_GATE_T5(t5_ao_i2c_slave,	AO_CLK_GATE0, 3);
MESON_AO_GATE_T5(t5_ao_uart1,		AO_CLK_GATE0, 4);
MESON_AO_GATE_T5(t5_ao_prod_i2c,	AO_CLK_GATE0, 5);
MESON_AO_GATE_T5(t5_ao_uart2,		AO_CLK_GATE0, 6);
MESON_AO_GATE_T5(t5_ao_ir_blaster,	AO_CLK_GATE0, 7);
MESON_AO_GATE_T5(t5_ao_sar_adc,		AO_CLK_GATE0, 8);

static struct clk_mux t5_aoclk81 = {
	.reg = (void *)AO_RTI_PWR_CNTL_REG0,
	.mask = 0x1,
	.shift = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_aoclk81",
		.ops = &clk_mux_ro_ops,
		.parent_names = (const char *[]){ "t5_clk81", "ao_slow_clk" },
		.num_parents = 2,
	},
};

static struct clk_mux t5_saradc_mux = {
	.reg = (void *)AO_SAR_CLK,
	.mask = 0x3,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_saradc_mux",
		.ops = &clk_mux_ops,
		.parent_names = (const char *[]){ "xtal", "t5_aoclk81"},
		.num_parents = 2,
		.flags = (CLK_GET_RATE_NOCACHE),
	},
};

static struct clk_divider t5_saradc_div = {
	.reg = (void *)AO_SAR_CLK,
	.shift = 0,
	.width = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_saradc_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_saradc_mux" },
		.num_parents = 1,
		.flags = (CLK_DIVIDER_ROUND_CLOSEST),
	},
};

static struct clk_gate t5_saradc_gate = {
	.reg = (void *)AO_SAR_CLK,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_saradc_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_saradc_div" },
		.num_parents = 1,
		.flags = (CLK_SET_RATE_PARENT),
	},
};

static const struct meson_clk_dualdiv_param clk_32k_table[] = {
	{
		.dual	= 1,
		.n1	= 733,
		.m1	= 8,
		.n2	= 732,
		.m2	= 11,
	},
	{}
};

static struct clk_gate t5_cecb_32k_clkin = {
	.reg = (void *)AO_CECB_CLK_CNTL_REG0,
	.bit_idx = 31,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "cecb_32k_clkin",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "xtal" },
		.num_parents = 1,
		.flags = CLK_IGNORE_UNUSED,
	},
};

static struct meson_dualdiv_clk t5_cecb_32k_div = {
	.data = &(struct meson_clk_dualdiv_data){
		.n1 = {
			.reg_off = AO_CECB_CLK_CNTL_REG0,
			.shift   = 0,
			.width   = 12,
		},
		.n2 = {
			.reg_off = AO_CECB_CLK_CNTL_REG0,
			.shift   = 12,
			.width   = 12,
		},
		.m1 = {
			.reg_off = AO_CECB_CLK_CNTL_REG1,
			.shift   = 0,
			.width   = 12,
		},
		.m2 = {
			.reg_off = AO_CECB_CLK_CNTL_REG1,
			.shift   = 12,
			.width   = 12,
		},
		.dual = {
			.reg_off = AO_CECB_CLK_CNTL_REG0,
			.shift   = 28,
			.width   = 1,
		},
		.table = clk_32k_table,
	},
	.hw.init = &(struct clk_init_data){
		.name = "cecb_32k_div",
		.ops = &meson_clk_dualdiv_ops,
		.parent_names = (const char *[]){ "cecb_32k_clkin" },
		.num_parents = 1,
	},
};

static struct clk_mux t5_cecb_32k_sel_pre = {
	.reg = (void *)AO_CECB_CLK_CNTL_REG1,
	.mask = 0x1,
	.shift = 24,
	.flags = CLK_MUX_ROUND_CLOSEST,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "cecb_32k_sel_pre",
		.ops = &clk_mux_ops,
		.parent_names = (const char *[]){ "cecb_32k_div",
						"cecb_32k_clkin" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_mux t5_cecb_32k_sel = {
	.reg = (void *)AO_CECB_CLK_CNTL_REG1,
	.mask = 0x1,
	.shift = 31,
	.flags = CLK_MUX_ROUND_CLOSEST,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "cecb_32k_sel",
		.ops = &clk_mux_ops,
		.parent_names = (const char *[]){ "cecb_32k_sel_pre",
						"rtc_clk" },
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_cecb_32k_clkout = {
	.reg = (void *)AO_CECB_CLK_CNTL_REG0,
	.bit_idx = 30,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "cecb_32k_clkout",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "cecb_32k_sel" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

/* Array of all clocks provided by this provider */
static struct clk_hw *t5_ao_clk_hws[] = {
	[CLKID_AO_CLK81] =	&t5_aoclk81.hw,
	[CLKID_SARADC_MUX] =	&t5_saradc_mux.hw,
	[CLKID_SARADC_DIV] =	&t5_saradc_div.hw,
	[CLKID_SARADC_GATE] =	&t5_saradc_gate.hw,
	[CLKID_AO_AHB_BUS] =	&t5_ao_ahb_bus.hw,
	[CLKID_AO_IR]	=	&t5_ao_ir.hw,
	[CLKID_AO_I2C_MASTER] = &t5_ao_i2c_master.hw,
	[CLKID_AO_I2C_SLAVE] =	&t5_ao_i2c_slave.hw,
	[CLKID_AO_UART1] =	&t5_ao_uart1.hw,
	[CLKID_AO_PROD_I2C] =	&t5_ao_prod_i2c.hw,
	[CLKID_AO_UART2] =	&t5_ao_uart2.hw,
	[CLKID_AO_IR_BLASTER] =	&t5_ao_ir_blaster.hw,
	[CLKID_AO_SAR_ADC] =	&t5_ao_sar_adc.hw,
	[CLKID_CECB_32K_CLKIN]	= &t5_cecb_32k_clkin.hw,
	[CLKID_CECB_32K_DIV]	= &t5_cecb_32k_div.hw,
	[CLKID_CECB_32K_MUX_PRE] = &t5_cecb_32k_sel_pre.hw,
	[CLKID_CECB_32K_MUX]	= &t5_cecb_32k_sel.hw,
	[CLKID_CECB_32K_CLKOUT]	= &t5_cecb_32k_clkout.hw
};

static struct clk_gate *t5_ao_clk_gates[] = {
	&t5_ao_ahb_bus,
	&t5_ao_ir,
	&t5_ao_i2c_master,
	&t5_ao_i2c_slave,
	&t5_ao_uart1,
	&t5_ao_prod_i2c,
	&t5_ao_uart2,
	&t5_ao_ir_blaster,
	&t5_ao_sar_adc,
	&t5_saradc_gate,
	&t5_cecb_32k_clkin,
	&t5_cecb_32k_clkout
};

static struct clk_onecell_data aoclk_data;

static int t5_aoclkc_probe(struct platform_device *pdev)
{
	void __iomem *aoclk_base;
	struct clk **ao_clks;
	int clkid, i, ret;
	struct device *dev = &pdev->dev;

	aoclk_base = of_iomap(dev->of_node, 0);
	if (!aoclk_base) {
		pr_err("%s: Unable to map clk base\n", __func__);
		return -ENXIO;
	}

	ao_clks = kcalloc(NR_AOCLKS, sizeof(struct clk *), GFP_KERNEL);
	aoclk_data.clks = ao_clks;
	aoclk_data.clk_num = NR_AOCLKS;

	for (i = 0; i < ARRAY_SIZE(t5_ao_clk_gates); i++)
		t5_ao_clk_gates[i]->reg = aoclk_base +
			(unsigned long)t5_ao_clk_gates[i]->reg;

	t5_aoclk81.reg = aoclk_base + (unsigned long)t5_aoclk81.reg;
	t5_saradc_mux.reg = aoclk_base + (unsigned long)t5_saradc_mux.reg;
	t5_saradc_div.reg = aoclk_base + (unsigned long)t5_saradc_div.reg;
	t5_cecb_32k_sel_pre.reg = aoclk_base + (unsigned long)t5_cecb_32k_sel_pre.reg;
	t5_cecb_32k_sel.reg = aoclk_base + (unsigned long)t5_cecb_32k_sel.reg;
	t5_cecb_32k_div.base = aoclk_base;

	for (clkid = 0; clkid < NR_AOCLKS; clkid++) {
		if (t5_ao_clk_hws[clkid]) {
			ao_clks[clkid] = clk_register(dev, t5_ao_clk_hws[clkid]);
			WARN_ON(IS_ERR(ao_clks[clkid]));
		}
	}

	ret = of_clk_add_provider(dev->of_node, of_clk_src_onecell_get, &aoclk_data);
	if (ret)
		pr_err("add AO clk provider failed\n");

	return 0;
}

static const struct of_device_id t5_aoclkc_match_table[] = {
	{ .compatible = "amlogic,t5-aoclkc" },
	{ },
};

static struct platform_driver t5_aoclk_driver = {
	.probe		= t5_aoclkc_probe,
	.driver		= {
		.name	= "t5-aoclkc",
		.of_match_table = t5_aoclkc_match_table,
	},
};

builtin_platform_driver(t5_aoclk_driver);

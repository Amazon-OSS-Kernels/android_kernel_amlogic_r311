/*
 * drivers/amlogic/clk/t5/t5.c
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
#include <linux/delay.h>
#include <dt-bindings/clock/amlogic,t5-clkc.h>
#include "../clkc.h"
#include "t5.h"

static struct clk_onecell_data clk_data;

static struct meson_clk_pll t5_fixed_pll = {
	.m = {
		.reg_off = HHI_FIX_PLL_CNTL0,
		.shift   = 0,
		.width   = 9,
	},
	.n = {
		.reg_off = HHI_FIX_PLL_CNTL0,
		.shift   = 10,
		.width   = 5,
	},
	.od = {
		.reg_off = HHI_FIX_PLL_CNTL0,
		.shift   = 16,
		.width   = 2,
	},
	.frac = {
		.reg_off = HHI_FIX_PLL_CNTL1,
		.shift   = 0,
		.width   = 19,
	},
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_fixed_pll",
		.ops = &meson_t5_pll_ro_ops,
		.parent_names = (const char *[]){ "xtal" },
		.num_parents = 1,
	},
};

static struct meson_clk_pll t5_sys_pll = {
	.m = {
		.reg_off = HHI_SYS_PLL_CNTL0,
		.shift   = 0,
		.width   = 9,
	},
	.n = {
		.reg_off = HHI_SYS_PLL_CNTL0,
		.shift   = 10,
		.width   = 5,
	},
	.od = {
		.reg_off = HHI_SYS_PLL_CNTL0,
		.shift   = 16,
		.width   = 3,
	},
	.rate_table = t5_pll_rate_table,
	.rate_count = ARRAY_SIZE(t5_pll_rate_table),
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_sys_pll",
		.ops = &meson_t5_pll_ops,
		.parent_names = (const char *[]){ "xtal" },
		.num_parents = 1,
		/*
		 * Do not disble sys pll by clk core, SYS PLL is set in U-boot.
		 * If sys pll is off, it will hang up.
		 * Do not set the flag as CLK_IS_CRITICAL, sys pll will turn
		 * off by cpufreq.
		 */
		.flags = CLK_IGNORE_UNUSED,
	},
};

static struct meson_clk_pll t5_gp0_pll = {
	.m = {
		.reg_off = HHI_GP0_PLL_CNTL0,
		.shift   = 0,
		.width   = 9,
	},
	.n = {
		.reg_off = HHI_GP0_PLL_CNTL0,
		.shift   = 10,
		.width   = 5,
	},
	.od = {
		.reg_off = HHI_GP0_PLL_CNTL0,
		.shift   = 16,
		.width   = 3,
	},
	.rate_table = t5_pll_rate_table,
	.rate_count = ARRAY_SIZE(t5_pll_rate_table),
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_gp0_pll",
		.ops = &meson_t5_pll_ops,
		.parent_names = (const char *[]){ "xtal" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_fixed_factor t5_fclk_div2 = {
	.mult = 1,
	.div = 2,
	.hw.init = &(struct clk_init_data){
		.name = "t5_fclk_div2",
		.ops = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "t5_fixed_pll" },
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t5_fclk_div3 = {
	.mult = 1,
	.div = 3,
	.hw.init = &(struct clk_init_data){
		.name = "t5_fclk_div3",
		.ops = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "t5_fixed_pll" },
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t5_fclk_div4 = {
	.mult = 1,
	.div = 4,
	.hw.init = &(struct clk_init_data){
		.name = "t5_fclk_div4",
		.ops = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "t5_fixed_pll" },
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t5_fclk_div5 = {
	.mult = 1,
	.div = 5,
	.hw.init = &(struct clk_init_data){
		.name = "t5_fclk_div5",
		.ops = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "t5_fixed_pll" },
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t5_fclk_div7 = {
	.mult = 1,
	.div = 7,
	.hw.init = &(struct clk_init_data){
		.name = "t5_fclk_div7",
		.ops = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "t5_fixed_pll" },
		.num_parents = 1,
	},
};

static struct clk_fixed_factor t5_fclk_div2p5 = {
	.mult = 2,
	.div = 5,
	.hw.init = &(struct clk_init_data){
		.name = "t5_fclk_div2p5",
		.ops = &clk_fixed_factor_ops,
		.parent_names = (const char *[]){ "t5_fixed_pll" },
		.num_parents = 1,
	},
};

static struct meson_clk_pll t5_hifi_pll = {
	.m = {
		.reg_off = HHI_HIFI_PLL_CNTL0,
		.shift   = 0,
		.width   = 9,
	},
	.n = {
		.reg_off = HHI_HIFI_PLL_CNTL0,
		.shift   = 10,
		.width   = 5,
	},
	.od = {
		.reg_off = HHI_HIFI_PLL_CNTL0,
		.shift   = 16,
		.width   = 2,
	},
	.rate_table = t5_hifi_pll_rate_table,
	.rate_count = ARRAY_SIZE(t5_hifi_pll_rate_table),
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hifi_pll",
		.ops = &meson_t5_pll_ops,
		.parent_names = (const char *[]){ "xtal" },
		.num_parents = 1,
	},
};

static struct meson_clk_mpll t5_mpll0 = {
	.sdm = {
		.reg_off = HHI_MPLL_CNTL1,
		.shift   = 0,
		.width   = 14,
	},
	.n2 = {
		.reg_off = HHI_MPLL_CNTL1,
		.shift   = 20,
		.width   = 9,
	},
	.sdm_en = 30,
	.en_dds = 31,
	.mpll_cntl0_reg = HHI_MPLL_CNTL0,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_mpll0",
		.ops = &meson_tl1_mpll_ops,
		.parent_names = (const char *[]){ "t5_fixed_pll" },
		.num_parents = 1,
	},
};

static struct meson_clk_mpll t5_mpll1 = {
	.sdm = {
		.reg_off = HHI_MPLL_CNTL3,
		.shift   = 0,
		.width   = 14,
	},
	.n2 = {
		.reg_off = HHI_MPLL_CNTL3,
		.shift   = 20,
		.width   = 9,
	},
	.sdm_en = 30,
	.en_dds = 31,
	.mpll_cntl0_reg = HHI_MPLL_CNTL0,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_mpll1",
		.ops = &meson_tl1_mpll_ops,
		.parent_names = (const char *[]){ "t5_fixed_pll" },
		.num_parents = 1,
	},
};

static struct meson_clk_mpll t5_mpll2 = {
	.sdm = {
		.reg_off = HHI_MPLL_CNTL5,
		.shift   = 0,
		.width   = 14,
	},
	.n2 = {
		.reg_off = HHI_MPLL_CNTL5,
		.shift   = 20,
		.width   = 9,
	},
	.sdm_en = 30,
	.en_dds = 31,
	.mpll_cntl0_reg = HHI_MPLL_CNTL0,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_mpll2",
		.ops = &meson_tl1_mpll_ops,
		.parent_names = (const char *[]){ "t5_fixed_pll" },
		.num_parents = 1,
	},
};

static struct meson_clk_mpll t5_mpll3 = {
	.sdm = {
		.reg_off = HHI_MPLL_CNTL7,
		.shift   = 0,
		.width   = 14,
	},
	.n2 = {
		.reg_off = HHI_MPLL_CNTL7,
		.shift   = 20,
		.width   = 9,
	},
	.sdm_en = 30,
	.en_dds = 31,
	.mpll_cntl0_reg = HHI_MPLL_CNTL0,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_mpll3",
		.ops = &meson_tl1_mpll_ops,
		.parent_names = (const char *[]){ "t5_fixed_pll" },
		.num_parents = 1,
	},
};

static u32 mux_table_cpu_p[] = { 0, 1, 2 };
static struct meson_cpu_mux_divider t5_cpu_fclk_p = {
	.reg = (void *)HHI_SYS_CPU_CLK_CNTL0,
	.cpu_fclk_p00 = {
		.mask = 0x3,
		.shift = 0,
		.width = 2,
	},
	.cpu_fclk_p0 = {
		.mask = 0x1,
		.shift = 2,
		.width = 1,
	},
	.cpu_fclk_p10 = {
		.mask = 0x3,
		.shift = 16,
		.width = 2,
	},
	.cpu_fclk_p1 = {
		.mask = 0x1,
		.shift = 18,
		.width = 1,
	},
	.cpu_fclk_p = {
		.mask = 0x1,
		.shift = 10,
		.width = 1,
	},
	.cpu_fclk_p01 = {
		.shift = 4,
		.width = 6,
	},
	.cpu_fclk_p11 = {
		.shift = 20,
		.width = 6,
	},
	.table = mux_table_cpu_p,
	.rate_table = fclk_pll_rate_table,
	.rate_count = ARRAY_SIZE(fclk_pll_rate_table),
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_cpu_fixedpll_p",
		.ops = &meson_fclk_cpu_ops,
		.parent_names = (const char *[]){ "xtal", "t5_fclk_div2",
			"t5_fclk_div3"},
		.num_parents = 3,
		.flags = (CLK_GET_RATE_NOCACHE | CLK_IGNORE_UNUSED),
	},
};

static struct clk_mux t5_cpu_clk = {
	.reg = (void *)HHI_SYS_CPU_CLK_CNTL0,
	.mask = 0x1,
	.shift = 11,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_cpu_clk",
		.ops = &clk_mux_ops,
		.parent_names = (const char *[]){ "t5_cpu_fixedpll_p",
						"t5_sys_pll" },
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

#define MESON_T5_SYS_GATE(_name, _reg, _bit)				\
struct clk_gate _name = {						\
	.reg = (void __iomem *)_reg,					\
	.bit_idx = (_bit),						\
	.lock = &clk_lock,						\
	.hw.init = &(struct clk_init_data) {				\
		.name = #_name,						\
		.ops = &clk_gate_ops,					\
		.parent_names = (const char *[]){ "t5_clk81" },		\
		.num_parents = 1,					\
		.flags = CLK_IGNORE_UNUSED,				\
	},								\
}

static MESON_T5_SYS_GATE(t5_clk81_ddr,		HHI_GCLK_MPEG0, 0);
static MESON_T5_SYS_GATE(t5_clk81_dos,		HHI_GCLK_MPEG0, 1);
static MESON_T5_SYS_GATE(t5_clk81_ethphy,	HHI_GCLK_MPEG0, 4);
static MESON_T5_SYS_GATE(t5_clk81_isa,		HHI_GCLK_MPEG0, 5);
static MESON_T5_SYS_GATE(t5_clk81_pl310,	HHI_GCLK_MPEG0, 6);
static MESON_T5_SYS_GATE(t5_clk81_periphs,	HHI_GCLK_MPEG0, 7);
static MESON_T5_SYS_GATE(t5_clk81_spicc0,	HHI_GCLK_MPEG0, 8);
static MESON_T5_SYS_GATE(t5_clk81_i2c,		HHI_GCLK_MPEG0, 9);
static MESON_T5_SYS_GATE(t5_clk81_sana,		HHI_GCLK_MPEG0, 10);
static MESON_T5_SYS_GATE(t5_clk81_smartcard,	HHI_GCLK_MPEG0, 11);
static MESON_T5_SYS_GATE(t5_clk81_uart0,	HHI_GCLK_MPEG0, 13);
static MESON_T5_SYS_GATE(t5_clk81_stream,	HHI_GCLK_MPEG0, 15);
static MESON_T5_SYS_GATE(t5_clk81_async_fifo,	HHI_GCLK_MPEG0, 16);
static MESON_T5_SYS_GATE(t5_clk81_tvfe,		HHI_GCLK_MPEG0, 18);
static MESON_T5_SYS_GATE(t5_clk81_hiu_reg,	HHI_GCLK_MPEG0, 19);
static MESON_T5_SYS_GATE(t5_clk81_hdmirx_pclk,	HHI_GCLK_MPEG0, 21);
static MESON_T5_SYS_GATE(t5_clk81_atv_demod,	HHI_GCLK_MPEG0, 22);
static MESON_T5_SYS_GATE(t5_clk81_assist_misc,	HHI_GCLK_MPEG0, 23);
static MESON_T5_SYS_GATE(t5_clk81_pwr_ctrl,	HHI_GCLK_MPEG0, 24);
static MESON_T5_SYS_GATE(t5_clk81_emmc_c,	HHI_GCLK_MPEG0, 26);
static MESON_T5_SYS_GATE(t5_clk81_adec,		HHI_GCLK_MPEG0, 27);
static MESON_T5_SYS_GATE(t5_clk81_acodec,	HHI_GCLK_MPEG0, 28);
static MESON_T5_SYS_GATE(t5_clk81_tcon,		HHI_GCLK_MPEG0, 29);
static MESON_T5_SYS_GATE(t5_clk81_spi,		HHI_GCLK_MPEG0, 30);

static MESON_T5_SYS_GATE(t5_clk81_audio,	HHI_GCLK_MPEG1, 0);
static MESON_T5_SYS_GATE(t5_clk81_eth,		HHI_GCLK_MPEG1, 3);
static MESON_T5_SYS_GATE(t5_clk81_top_demux,	HHI_GCLK_MPEG1, 4);
static MESON_T5_SYS_GATE(t5_clk81_clk_rst,	HHI_GCLK_MPEG1, 5);
static MESON_T5_SYS_GATE(t5_clk81_aififo,	HHI_GCLK_MPEG1, 11);
static MESON_T5_SYS_GATE(t5_clk81_uart1,	HHI_GCLK_MPEG1, 16);
static MESON_T5_SYS_GATE(t5_clk81_g2d,		HHI_GCLK_MPEG1, 20);
static MESON_T5_SYS_GATE(t5_clk81_reset,	HHI_GCLK_MPEG1, 23);
static MESON_T5_SYS_GATE(t5_clk81_parser0,	HHI_GCLK_MPEG1, 25);
static MESON_T5_SYS_GATE(t5_clk81_usb_gene,	HHI_GCLK_MPEG1, 26);
static MESON_T5_SYS_GATE(t5_clk81_parser1,	HHI_GCLK_MPEG1, 28);
static MESON_T5_SYS_GATE(t5_clk81_ahb_arb0,	HHI_GCLK_MPEG1, 29);

static MESON_T5_SYS_GATE(t5_clk81_ahb_data,	HHI_GCLK_MPEG2, 1);
static MESON_T5_SYS_GATE(t5_clk81_ahb_ctrl,	HHI_GCLK_MPEG2, 2);
static MESON_T5_SYS_GATE(t5_clk81_usb1_to_ddr,	HHI_GCLK_MPEG2, 8);
static MESON_T5_SYS_GATE(t5_clk81_mmc_pclk,	HHI_GCLK_MPEG2, 11);
static MESON_T5_SYS_GATE(t5_clk81_hdmirx_axi,	HHI_GCLK_MPEG2, 12);
static MESON_T5_SYS_GATE(t5_clk81_hdcp22_pclk,	HHI_GCLK_MPEG2, 13);
static MESON_T5_SYS_GATE(t5_clk81_uart2,	HHI_GCLK_MPEG2, 15);
static MESON_T5_SYS_GATE(t5_clk81_ciplus,	HHI_GCLK_MPEG2, 21);
static MESON_T5_SYS_GATE(t5_clk81_ts,		HHI_GCLK_MPEG2, 22);
static MESON_T5_SYS_GATE(t5_clk81_vpu_int,	HHI_GCLK_MPEG2, 25);
static MESON_T5_SYS_GATE(t5_clk81_demod_com,	HHI_GCLK_MPEG2, 28);
static MESON_T5_SYS_GATE(t5_clk81_gic,		HHI_GCLK_MPEG2, 30);

static MESON_T5_SYS_GATE(t5_clk81_vclk2_venci0,	HHI_GCLK_OTHER, 1);
static MESON_T5_SYS_GATE(t5_clk81_vclk2_venci1,	HHI_GCLK_OTHER, 2);
static MESON_T5_SYS_GATE(t5_clk81_vclk2_vencp0,	HHI_GCLK_OTHER, 3);
static MESON_T5_SYS_GATE(t5_clk81_vclk2_vencp1,	HHI_GCLK_OTHER, 4);
static MESON_T5_SYS_GATE(t5_clk81_vclk2_venct0,	HHI_GCLK_OTHER, 5);
static MESON_T5_SYS_GATE(t5_clk81_vclk2_venct1,	HHI_GCLK_OTHER, 6);
static MESON_T5_SYS_GATE(t5_clk81_vclk2_other,	HHI_GCLK_OTHER, 7);
static MESON_T5_SYS_GATE(t5_clk81_vclk2_enci,	HHI_GCLK_OTHER, 8);
static MESON_T5_SYS_GATE(t5_clk81_vclk2_encp,	HHI_GCLK_OTHER, 9);
static MESON_T5_SYS_GATE(t5_clk81_dac_clk,	HHI_GCLK_OTHER, 10);
static MESON_T5_SYS_GATE(t5_clk81_enc480p,	HHI_GCLK_OTHER, 20);
static MESON_T5_SYS_GATE(t5_clk81_ramdom,	HHI_GCLK_OTHER, 21);
static MESON_T5_SYS_GATE(t5_clk81_vclk2_enct,	HHI_GCLK_OTHER, 22);
static MESON_T5_SYS_GATE(t5_clk81_vclk2_encl,	HHI_GCLK_OTHER, 23);
static MESON_T5_SYS_GATE(t5_clk81_vclk2_venclmmc,	HHI_GCLK_OTHER, 24);
static MESON_T5_SYS_GATE(t5_clk81_vclk2_vencl,	HHI_GCLK_OTHER, 25);
static MESON_T5_SYS_GATE(t5_clk81_vclk2_other1,	HHI_GCLK_OTHER, 26);

static MESON_T5_SYS_GATE(t5_clk81_dma,		HHI_GCLK_AO, 0);
static MESON_T5_SYS_GATE(t5_clk81_efuse,	HHI_GCLK_AO, 1);
static MESON_T5_SYS_GATE(t5_clk81_rom_boot,	HHI_GCLK_AO, 2);
static MESON_T5_SYS_GATE(t5_clk81_reset_sec,	HHI_GCLK_AO, 3);
static MESON_T5_SYS_GATE(t5_clk81_sec_ahb,	HHI_GCLK_AO, 4);
static MESON_T5_SYS_GATE(t5_clk81_rsa,		HHI_GCLK_AO, 5);

/* Array of all clocks provided by this provider */
static struct clk_hw *t5_clk_hws[] = {
	[CLKID_FIXED_PLL]	= &t5_fixed_pll.hw,
	[CLKID_SYS_PLL]		= &t5_sys_pll.hw,
	[CLKID_GP0_PLL]		= &t5_gp0_pll.hw,
	[CLKID_HIFI_PLL]	= &t5_hifi_pll.hw,
	[CLKID_FCLK_DIV2]	= &t5_fclk_div2.hw,
	[CLKID_FCLK_DIV3]	= &t5_fclk_div3.hw,
	[CLKID_FCLK_DIV4]	= &t5_fclk_div4.hw,
	[CLKID_FCLK_DIV5]	= &t5_fclk_div5.hw,
	[CLKID_FCLK_DIV7]	= &t5_fclk_div7.hw,
	[CLKID_FCLK_DIV2P5]	= &t5_fclk_div2p5.hw,
	[CLKID_MPLL0]           = &t5_mpll0.hw,
	[CLKID_MPLL1]           = &t5_mpll1.hw,
	[CLKID_MPLL2]           = &t5_mpll2.hw,
	[CLKID_MPLL3]           = &t5_mpll3.hw,
	[CLKID_CPU_FCLK_P]      = &t5_cpu_fclk_p.hw,
	[CLKID_CPU_CLK]         = &t5_cpu_clk.hw,
	[CLKID_DDR]		= &t5_clk81_ddr.hw,
	[CLKID_DOS]		= &t5_clk81_dos.hw,
	[CLKID_ETH_PHY]		= &t5_clk81_ethphy.hw,
	[CLKID_ISA]		= &t5_clk81_isa.hw,
	[CLKID_PL310]		= &t5_clk81_pl310.hw,
	[CLKID_PERIPHS]		= &t5_clk81_periphs.hw,
	[CLKID_SPICC0]		= &t5_clk81_spicc0.hw,
	[CLKID_I2C]		= &t5_clk81_i2c.hw,
	[CLKID_SANA]		= &t5_clk81_sana.hw,
	[CLKID_SMARTCARD]	= &t5_clk81_smartcard.hw,
	[CLKID_UART0]		= &t5_clk81_uart0.hw,
	[CLKID_STREAM]		= &t5_clk81_stream.hw,
	[CLKID_ASYNC_FIFO]	= &t5_clk81_async_fifo.hw,
	[CLKID_TVFE]		= &t5_clk81_tvfe.hw,
	[CLKID_HIU_REG]		= &t5_clk81_hiu_reg.hw,
	[CLKID_HDMIRX_PCLK]	= &t5_clk81_hdmirx_pclk.hw,
	[CLKID_ATV_DEMOD]	= &t5_clk81_atv_demod.hw,
	[CLKID_ASSIST_MISC]	= &t5_clk81_assist_misc.hw,
	[CLKID_PWR_CTRL]	= &t5_clk81_pwr_ctrl.hw,
	[CLKID_SD_EMMC_C]	= &t5_clk81_emmc_c.hw,
	[CLKID_ADEC]		= &t5_clk81_adec.hw,
	[CLKID_ACODEC]		= &t5_clk81_acodec.hw,
	[CLKID_TCON]		= &t5_clk81_tcon.hw,
	[CLKID_SPI]		= &t5_clk81_spi.hw,
	[CLKID_AUDIO]		= &t5_clk81_audio.hw,
	[CLKID_ETH_CORE]	= &t5_clk81_eth.hw,
	[CLKID_DEMUX]		= &t5_clk81_top_demux.hw,
	[CLKID_CLK_RST]		= &t5_clk81_clk_rst.hw,
	[CLKID_AIFIFO]		= &t5_clk81_aififo.hw,
	[CLKID_UART1]		= &t5_clk81_uart1.hw,
	[CLKID_G2D]		= &t5_clk81_g2d.hw,
	[CLKID_RESET]		= &t5_clk81_reset.hw,
	[CLKID_PARSER0]		= &t5_clk81_parser0.hw,
	[CLKID_USB_GENERAL]	= &t5_clk81_usb_gene.hw,
	[CLKID_PARSER1]		= &t5_clk81_parser1.hw,
	[CLKID_AHB_ARB0]	= &t5_clk81_ahb_arb0.hw,
	[CLKID_AHB_DATA_BUS]	= &t5_clk81_ahb_data.hw,
	[CLKID_AHB_CTRL_BUS]	= &t5_clk81_ahb_ctrl.hw,
	[CLKID_USB1_TO_DDR]	= &t5_clk81_usb1_to_ddr.hw,
	[CLKID_MMC_PCLK]	= &t5_clk81_mmc_pclk.hw,
	[CLKID_HDMIRX_AXI]	= &t5_clk81_hdmirx_axi.hw,
	[CLKID_HDCP22_PCLK]	= &t5_clk81_hdcp22_pclk.hw,
	[CLKID_UART2]		= &t5_clk81_uart2.hw,
	[CLKID_CIPLUS]		= &t5_clk81_ciplus.hw,
	[CLKID_CLK81_TS]	= &t5_clk81_ts.hw,
	[CLKID_VPU_INTR]	= &t5_clk81_vpu_int.hw,
	[CLKID_DEMOD_COMB]	= &t5_clk81_demod_com.hw,
	[CLKID_GIC]		= &t5_clk81_gic.hw,
	[CLKID_VCLK2_VENCI0]	= &t5_clk81_vclk2_venci0.hw,
	[CLKID_VCLK2_VENCI1]	= &t5_clk81_vclk2_venci1.hw,
	[CLKID_VCLK2_VENCP0]	= &t5_clk81_vclk2_vencp0.hw,
	[CLKID_VCLK2_VENCP1]	= &t5_clk81_vclk2_vencp1.hw,
	[CLKID_VCLK2_VENCT0]	= &t5_clk81_vclk2_venct0.hw,
	[CLKID_VCLK2_VENCT1]	= &t5_clk81_vclk2_venct1.hw,
	[CLKID_VCLK2_OTHER]	= &t5_clk81_vclk2_other.hw,
	[CLKID_VCLK2_ENCI]	= &t5_clk81_vclk2_enci.hw,
	[CLKID_VCLK2_ENCP]	= &t5_clk81_vclk2_encp.hw,
	[CLKID_DAC_CLK]		= &t5_clk81_dac_clk.hw,
	[CLKID_ENC480P]		= &t5_clk81_enc480p.hw,
	[CLKID_RAMDOM]		= &t5_clk81_ramdom.hw,
	[CLKID_VCLK2_ENCT]	= &t5_clk81_vclk2_enct.hw,
	[CLKID_VCLK2_ENCL]	= &t5_clk81_vclk2_encl.hw,
	[CLKID_VCLK2_VENCLMMC]	= &t5_clk81_vclk2_venclmmc.hw,
	[CLKID_VCLK2_VENCL]	= &t5_clk81_vclk2_vencl.hw,
	[CLKID_VCLK2_OTHER1]	= &t5_clk81_vclk2_other1.hw,
	[CLKID_DMA]		= &t5_clk81_dma.hw,
	[CLKID_EFUSE]		= &t5_clk81_efuse.hw,
	[CLKID_ROM_BOOT]	= &t5_clk81_rom_boot.hw,
	[CLKID_RESET_SEC]	= &t5_clk81_reset_sec.hw,
	[CLKID_SEC_AHB]		= &t5_clk81_sec_ahb.hw,
	[CLKID_RSA]		= &t5_clk81_rsa.hw,/* clk81 gate over */
};

/* Convenience tables to populate base addresses in .probe */
static struct meson_clk_pll *const t5_clk_plls[] = {
	&t5_fixed_pll,
	&t5_sys_pll,
	&t5_gp0_pll,
	&t5_hifi_pll,
};

static struct meson_clk_mpll *const t5_clk_mplls[] = {
	&t5_mpll0,
	&t5_mpll1,
	&t5_mpll2,
	&t5_mpll3,
};

static struct clk_gate *t5_clk_gates[] = {
	&t5_clk81_ddr,
	&t5_clk81_dos,
	&t5_clk81_ethphy,
	&t5_clk81_isa,
	&t5_clk81_pl310,
	&t5_clk81_periphs,
	&t5_clk81_spicc0,
	&t5_clk81_i2c,
	&t5_clk81_sana,
	&t5_clk81_smartcard,
	&t5_clk81_uart0,
	&t5_clk81_stream,
	&t5_clk81_async_fifo,
	&t5_clk81_tvfe,
	&t5_clk81_hiu_reg,
	&t5_clk81_hdmirx_pclk,
	&t5_clk81_atv_demod,
	&t5_clk81_assist_misc,
	&t5_clk81_pwr_ctrl,
	&t5_clk81_emmc_c,
	&t5_clk81_adec,
	&t5_clk81_acodec,
	&t5_clk81_tcon,
	&t5_clk81_spi,
	&t5_clk81_audio,
	&t5_clk81_eth,
	&t5_clk81_top_demux,
	&t5_clk81_clk_rst,
	&t5_clk81_aififo,
	&t5_clk81_uart1,
	&t5_clk81_g2d,
	&t5_clk81_reset,
	&t5_clk81_parser0,
	&t5_clk81_usb_gene,
	&t5_clk81_parser1,
	&t5_clk81_ahb_arb0,
	&t5_clk81_ahb_data,
	&t5_clk81_ahb_ctrl,
	&t5_clk81_usb1_to_ddr,
	&t5_clk81_mmc_pclk,
	&t5_clk81_hdmirx_axi,
	&t5_clk81_hdcp22_pclk,
	&t5_clk81_uart2,
	&t5_clk81_ciplus,
	&t5_clk81_ts,
	&t5_clk81_vpu_int,
	&t5_clk81_demod_com,
	&t5_clk81_gic,
	&t5_clk81_vclk2_venci0,
	&t5_clk81_vclk2_venci1,
	&t5_clk81_vclk2_vencp0,
	&t5_clk81_vclk2_vencp1,
	&t5_clk81_vclk2_venct0,
	&t5_clk81_vclk2_venct1,
	&t5_clk81_vclk2_other,
	&t5_clk81_vclk2_enci,
	&t5_clk81_vclk2_encp,
	&t5_clk81_dac_clk,
	&t5_clk81_enc480p,
	&t5_clk81_ramdom,
	&t5_clk81_vclk2_enct,
	&t5_clk81_vclk2_encl,
	&t5_clk81_vclk2_venclmmc,
	&t5_clk81_vclk2_vencl,
	&t5_clk81_vclk2_other1,
	&t5_clk81_dma,
	&t5_clk81_efuse,
	&t5_clk81_rom_boot,
	&t5_clk81_reset_sec,
	&t5_clk81_sec_ahb,
	&t5_clk81_rsa,/* clk81 gate over */
};

static int t5_cpu_clk_notifier_cb(struct notifier_block *nb,
				  unsigned long event, void *data)
{
	struct clk_hw **hws = t5_clk_hws;
	struct clk_hw *cpu_clk_hw, *parent_clk_hw;
	struct clk *cpu_clk, *parent_clk;
	int ret, control, cnt = 0;

	switch (event) {
	case PRE_RATE_CHANGE:
		parent_clk_hw = hws[CLKID_CPU_FCLK_P];
		break;
	case POST_RATE_CHANGE:
		parent_clk_hw = &t5_sys_pll.hw;
		break;
	default:
		return NOTIFY_DONE;
	}

	/* Waiting the cpu fsm idle, than set bit11 0 or 1 */
	do {
		control = readl(t5_cpu_clk.reg);
		udelay(10);
		cnt++;
		if (cnt >= 100*100)/*100ms*/ {
			pr_err("The cpu FSM is busy\n");
		}
	} while (control & (1 << 28));

	cpu_clk_hw = hws[CLKID_CPU_CLK];
	cpu_clk = __clk_lookup(clk_hw_get_name(cpu_clk_hw));
	parent_clk = __clk_lookup(clk_hw_get_name(parent_clk_hw));

	ret = clk_set_parent(cpu_clk, parent_clk);
	if (ret)
		return notifier_from_errno(ret);

	udelay(80);

	return NOTIFY_OK;
}

static struct notifier_block t5_cpu_nb_data = {
	.notifier_call = t5_cpu_clk_notifier_cb,
};

static void __init t5_clkc_init(struct device_node *np)
{
	int ret = 0, clkid, i;
	void __iomem *basic_clk;

	basic_clk = of_iomap(np, 0);
	if (!basic_clk)
		return;

	/* Populate base address for PLLs */
	for (i = 0; i < ARRAY_SIZE(t5_clk_plls); i++)
		t5_clk_plls[i]->base = basic_clk;

	/* Populate base address for MPLLs */
	for (i = 0; i < ARRAY_SIZE(t5_clk_mplls); i++)
		t5_clk_mplls[i]->base = basic_clk;

	/* Populate the base address for CPU clk */
	t5_cpu_fclk_p.reg = basic_clk
		+ (unsigned long)t5_cpu_fclk_p.reg;
	t5_cpu_clk.reg = basic_clk
		+ (unsigned long)t5_cpu_clk.reg;

	/* Populate base address for gates */
	for (i = 0; i < ARRAY_SIZE(t5_clk_gates); i++)
		t5_clk_gates[i]->reg = basic_clk +
			(unsigned long)t5_clk_gates[i]->reg;

	clks = kcalloc(NR_CLKS, sizeof(struct clk *), GFP_KERNEL);

	clk_numbers = NR_CLKS;
	clk_data.clks = clks;
	clk_data.clk_num = NR_CLKS;

	/*
	 * register all clks
	 */
	for (clkid = 0; clkid < ARRAY_SIZE(t5_clk_hws); clkid++) {
		if (t5_clk_hws[clkid]) {
			clks[clkid] = clk_register(NULL, t5_clk_hws[clkid]);
			if (IS_ERR(clks[clkid])) {
				pr_err("%s: failed to register %s\n", __func__,
				       clk_hw_get_name(t5_clk_hws[clkid]));
				goto iounmap;
			}
		}
	}

	/*
	 * Register CPU clk notifier
	 *
	 * FIXME this is wrong for a lot of reasons. First, the muxes should be
	 * struct clk_hw objects. Second, we shouldn't program the muxes in
	 * notifier handlers. The tricky programming sequence will be handled
	 * by the forthcoming coordinated clock rates mechanism once that
	 * feature is released.
	 *
	 * Furthermore, looking up the parent this way is terrible. At some
	 * point we will stop allocating a default struct clk when registering
	 * a new clk_hw, and this hack will no longer work. Releasing the ccr
	 * feature before that time solves the problem :-)
	 */
	ret = clk_notifier_register(t5_sys_pll.hw.clk, &t5_cpu_nb_data);
	if (ret) {
		pr_err("%s: failed to register clock notifier for cpu_clk\n",
		       __func__);
		goto iounmap;
	}

	ret = of_clk_add_provider(np, of_clk_src_onecell_get,
				  &clk_data);
	if (ret < 0)
		pr_err("%s fail ret: %d\n", __func__, ret);

	return;
iounmap:
	iounmap(basic_clk);
}

CLK_OF_DECLARE(t5, "amlogic,t5-clkc", t5_clkc_init);

/*
 * drivers/amlogic/clk/t5/t5_peripheral.c
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

#include "../clkc.h"
#include "t5_peripheral.h"

static u32 mux_table_clk81[]	= { 6, 5, 7 };

static struct clk_mux t5_mpeg_clk_sel = {
	.reg = (void *)HHI_MPEG_CLK_CNTL,
	.mask = 0x7,
	.shift = 12,
	.flags = CLK_MUX_READ_ONLY,
	.table = mux_table_clk81,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_mpeg_clk_sel",
		.ops = &clk_mux_ro_ops,
		/*
		 * FIXME bits 14:12 selects from 8 possible parents:
		 * xtal, 1'b0 (wtf), fclk_div7, mpll_clkout1, mpll_clkout2,
		 * fclk_div4, fclk_div3, fclk_div5
		 */
		.parent_names = (const char *[]){ "t5_fclk_div3", "t5_fclk_div4",
			"t5_fclk_div5" },
		.num_parents = 3,
	},
};

static struct clk_divider t5_mpeg_clk_div = {
	.reg = (void *)HHI_MPEG_CLK_CNTL,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_mpeg_clk_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_mpeg_clk_sel" },
		.num_parents = 1,
	},
};

static struct clk_gate t5_clk81 = {
	.reg = (void *)HHI_MPEG_CLK_CNTL,
	.bit_idx = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_clk81",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_mpeg_clk_div" },
		.num_parents = 1,
		.flags = CLK_IGNORE_UNUSED,
	},
};

static struct clk_divider t5_ts_div = {
	.reg = (void *)HHI_TS_CLK_CNTL,
	.shift = 0,
	.width = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_ts_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "xtal" },
		.num_parents = 1,
	},
};

static struct clk_gate t5_ts = {
	.reg = (void *)HHI_TS_CLK_CNTL,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_ts",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_ts_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

const char *t5_gpu_parent_names[] = { "xtal", "t5_gp0_pll", "t5_hifi_pll",
	"t5_fclk_div2p5", "t5_fclk_div3", "t5_fclk_div4", "t5_fclk_div5", "t5_fclk_div7"};

static struct clk_mux t5_gpu_p0_mux = {
	.reg = (void *)HHI_MALI_CLK_CNTL,
	.mask = 0x7,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_gpu_p0_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_gpu_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_gpu_p0_div = {
	.reg = (void *)HHI_MALI_CLK_CNTL,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_gpu_p0_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_gpu_p0_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_gpu_p0_gate = {
	.reg = (void *)HHI_MALI_CLK_CNTL,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_gpu_p0_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_gpu_p0_div" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_mux t5_gpu_p1_mux = {
	.reg = (void *)HHI_MALI_CLK_CNTL,
	.mask = 0x7,
	.shift = 25,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_gpu_p1_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_gpu_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_gpu_p1_div = {
	.reg = (void *)HHI_MALI_CLK_CNTL,
	.shift = 16,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_gpu_p1_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_gpu_p1_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_gpu_p1_gate = {
	.reg = (void *)HHI_MALI_CLK_CNTL,
	.bit_idx = 24,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_gpu_p1_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_gpu_p1_div" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT
	},
};

static struct clk_mux t5_gpu_mux = {
	.reg = (void *)HHI_MALI_CLK_CNTL,
	.mask = 0x1,
	.shift = 31,
	.lock = &clk_lock,
	.flags = CLK_PARENT_ALTERNATE,
	.hw.init = &(struct clk_init_data){
		.name = "t5_gpu_mux",
		.ops = &meson_clk_mux_ops,
		.parent_names = (const char *[]){ "t5_gpu_p0_gate",
			"t5_gpu_p1_gate"},
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_UNGATE,
	},
};

static const char * const t5_vpu_parent_names[] = { "t5_fclk_div3",
	"t5_fclk_div4", "t5_fclk_div5", "t5_fclk_div7", "null", "null",
	"null",  "null"};

static struct clk_mux t5_vpu_p0_mux = {
	.reg = (void *)HHI_VPU_CLK_CNTL,
	.mask = 0x7,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vpu_p0_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_vpu_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_vpu_p0_div = {
	.reg = (void *)HHI_VPU_CLK_CNTL,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vpu_p0_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_vpu_p0_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_vpu_p0_gate = {
	.reg = (void *)HHI_VPU_CLK_CNTL,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_vpu_p0_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_vpu_p0_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_mux t5_vpu_p1_mux = {
	.reg = (void *)HHI_VPU_CLK_CNTL,
	.mask = 0x7,
	.shift = 25,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vpu_p1_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_vpu_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_vpu_p1_div = {
	.reg = (void *)HHI_VPU_CLK_CNTL,
	.shift = 16,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vpu_p1_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_vpu_p1_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_vpu_p1_gate = {
	.reg = (void *)HHI_VPU_CLK_CNTL,
	.bit_idx = 24,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_vpu_p1_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_vpu_p1_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_mux t5_vpu_mux = {
	.reg = (void *)HHI_VPU_CLK_CNTL,
	.mask = 0x1,
	.shift = 31,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vpu_mux",
		.ops = &clk_mux_ops,
		.parent_names = (const char *[]){ "t5_vpu_p0_gate",
			"t5_vpu_p1_gate"},
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

/* vpu clkc clock, different with vpu clock */
static const char * const t5_vpu_clkc_parent_names[] = { "t5_fclk_div4",
	"t5_fclk_div3", "t5_fclk_div5", "t5_fclk_div7", "null", "null",
	"null",  "null"};

static struct clk_mux t5_vpu_clkc_p0_mux = {
	.reg = (void *)HHI_VPU_CLKC_CNTL,
	.mask = 0x7,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vpu_clkc_p0_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_vpu_clkc_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_vpu_clkc_p0_div = {
	.reg = (void *)HHI_VPU_CLKC_CNTL,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vpu_clkc_p0_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_vpu_clkc_p0_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_vpu_clkc_p0_gate = {
	.reg = (void *)HHI_VPU_CLKC_CNTL,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_vpu_clkc_p0_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_vpu_clkc_p0_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_mux t5_vpu_clkc_p1_mux = {
	.reg = (void *)HHI_VPU_CLKC_CNTL,
	.mask = 0x7,
	.shift = 25,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vpu_clkc_p1_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_vpu_clkc_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_vpu_clkc_p1_div = {
	.reg = (void *)HHI_VPU_CLKC_CNTL,
	.shift = 16,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vpu_clkc_p1_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_vpu_clkc_p1_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_vpu_clkc_p1_gate = {
	.reg = (void *)HHI_VPU_CLKC_CNTL,
	.bit_idx = 24,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_vpu_clkc_p1_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_vpu_clkc_p1_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_mux t5_vpu_clkc_mux = {
	.reg = (void *)HHI_VPU_CLKC_CNTL,
	.mask = 0x1,
	.shift = 31,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vpu_clkc_mux",
		.ops = &meson_clk_mux_ops,
		.parent_names = (const char *[]){ "t5_vpu_clkc_p0_gate",
			"t5_vpu_clkc_p1_gate"},
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static const char * const t5_adc_extclk_in_parent_names[] = { "xtal",
	"t5_fclk_div4", "t5_fclk_div3", "t5_fclk_div5",
	"t5_fclk_div7", "t5_mpll2", "t5_gp0_pll", "t5_hifi_pll" };

static struct clk_mux t5_adc_extclk_in_mux = {
	.reg = (void *)HHI_DEMOD_CLK_CNTL,
	.mask = 0x7,
	.shift = 25,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_adc_extclk_in_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_adc_extclk_in_parent_names,
		.num_parents = ARRAY_SIZE(t5_adc_extclk_in_parent_names),
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_adc_extclk_in_div = {
	.reg = (void *)HHI_DEMOD_CLK_CNTL,
	.shift = 16,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_adc_extclk_in_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_adc_extclk_in_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_adc_extclk_in = {
	.reg = (void *)HHI_DEMOD_CLK_CNTL,
	.bit_idx = 24,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_adc_extclk_in",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_adc_extclk_in_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static const char * const t5_demod_parent_names[] = { "xtal",
	"t5_fclk_div7", "t5_fclk_div4", "t5_adc_extclk_in" };

static struct clk_mux t5_demod_core_clk_mux = {
	.reg = (void *)HHI_DEMOD_CLK_CNTL,
	.mask = 0x3,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_demod_core_clk_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_demod_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_demod_core_clk_div = {
	.reg = (void *)HHI_DEMOD_CLK_CNTL,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_demod_core_clk_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_demod_core_clk_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_demod_core_clk = {
	.reg = (void *)HHI_DEMOD_CLK_CNTL,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_demod_core_clk",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_demod_core_clk_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

const char *t5_dec_parent_names[] = { "t5_fclk_div2p5", "t5_fclk_div3",
	"t5_fclk_div4", "t5_fclk_div5", "t5_fclk_div7", "t5_hifi_pll", "t5_gp0_pll", "xtal"};

static struct clk_mux t5_vdec_p0_mux = {
	.reg = (void *)HHI_VDEC_CLK_CNTL,
	.mask = 0x7,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vdec_p0_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_dec_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_vdec_p0_div = {
	.reg = (void *)HHI_VDEC_CLK_CNTL,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vdec_p0_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_vdec_p0_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_vdec_p0_gate = {
	.reg = (void *)HHI_VDEC_CLK_CNTL,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_vdec_p0_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_vdec_p0_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_mux t5_hevcf_p0_mux = {
	.reg = (void *)HHI_VDEC2_CLK_CNTL,
	.mask = 0x7,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hevcf_p0_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_dec_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_hevcf_p0_div = {
	.reg = (void *)HHI_VDEC2_CLK_CNTL,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hevcf_p0_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_hevcf_p0_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_hevcf_p0_gate = {
	.reg = (void *)HHI_VDEC2_CLK_CNTL,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_hevcf_p0_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_hevcf_p0_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_mux t5_vdec_p1_mux = {
	.reg = (void *)HHI_VDEC3_CLK_CNTL,
	.mask = 0x7,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vdec_p1_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_dec_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_vdec_p1_div = {
	.reg = (void *)HHI_VDEC3_CLK_CNTL,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vdec_p1_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_vdec_p1_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_vdec_p1_gate = {
	.reg = (void *)HHI_VDEC3_CLK_CNTL,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_vdec_p1_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_vdec_p1_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_mux t5_vdec_mux = {
	.reg = (void *)HHI_VDEC3_CLK_CNTL,
	.mask = 0x1,
	.shift = 15,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vdec_mux",
		.ops = &clk_mux_ops,
		.parent_names = (const char *[]){ "t5_vdec_p0_gate",
					"t5_vdec_p1_gate"},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_mux t5_hevcf_p1_mux = {
	.reg = (void *)HHI_VDEC4_CLK_CNTL,
	.mask = 0x7,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hevcf_p1_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_dec_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_hevcf_p1_div = {
	.reg = (void *)HHI_VDEC4_CLK_CNTL,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hevcf_p1_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_hevcf_p1_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_hevcf_p1_gate = {
	.reg = (void *)HHI_VDEC4_CLK_CNTL,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_hevcf_p1_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_hevcf_p1_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_mux t5_hevcf_mux = {
	.reg = (void *)HHI_VDEC4_CLK_CNTL,
	.mask = 0x1,
	.shift = 15,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hevcf_mux",
		.ops = &clk_mux_ops,
		.parent_names = (const char *[]){ "t5_hevcf_p0_gate",
					"t5_hevcf_p1_gate"},
		.num_parents = 2,
		.flags = CLK_SET_RATE_PARENT | CLK_GET_RATE_NOCACHE,
	},
};

static const char * const t5_hdcp22_esm_parent_names[] = { "t5_fclk_div7",
	"t5_fclk_div4", "t5_fclk_div3", "t5_fclk_div5" };

static struct clk_mux t5_hdcp22_esm_mux = {
	.reg = (void *)HHI_HDCP22_CLK_CNTL,
	.mask = 0x3,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hdcp22_esm_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_hdcp22_esm_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_hdcp22_esm_div = {
	.reg = (void *)HHI_HDCP22_CLK_CNTL,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hdcp22_esm_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_hdcp22_esm_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_hdcp22_esm_gate = {
	.reg = (void *)HHI_HDCP22_CLK_CNTL,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_hdcp22_esm_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_hdcp22_esm_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static const char * const t5_hdcp22_skp_parent_names[] = { "xtal",
	"t5_fclk_div4", "t5_fclk_div3", "t5_fclk_div5" };

static struct clk_mux t5_hdcp22_skp_mux = {
	.reg = (void *)HHI_HDCP22_CLK_CNTL,
	.mask = 0x3,
	.shift = 25,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hdcp22_skp_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_hdcp22_skp_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_hdcp22_skp_div = {
	.reg = (void *)HHI_HDCP22_CLK_CNTL,
	.shift = 16,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hdcp22_skp_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_hdcp22_skp_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_hdcp22_skp_gate = {
	.reg = (void *)HHI_HDCP22_CLK_CNTL,
	.bit_idx = 24,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_hdcp22_skp_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_hdcp22_skp_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static const char * const t5_vapb_parent_names[] = { "t5_fclk_div4",
	"t5_fclk_div3", "t5_fclk_div5", "t5_fclk_div7", "t5_mpll1", "null",
	"t5_mpll2",  "t5_fclk_div2p5"};

static struct clk_mux t5_vapb_p0_mux = {
	.reg = (void *)HHI_VAPBCLK_CNTL,
	.mask = 0x7,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vapb_p0_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_vapb_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_vapb_p0_div = {
	.reg = (void *)HHI_VAPBCLK_CNTL,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vapb_p0_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_vapb_p0_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_vapb_p0_gate = {
	.reg = (void *)HHI_VAPBCLK_CNTL,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_vapb_p0_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_vapb_p0_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_mux t5_vapb_p1_mux = {
	.reg = (void *)HHI_VAPBCLK_CNTL,
	.mask = 0x7,
	.shift = 25,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vapb_p1_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_vapb_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_vapb_p1_div = {
	.reg = (void *)HHI_VAPBCLK_CNTL,
	.shift = 16,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vapb_p1_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_vapb_p1_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_vapb_p1_gate = {
	.reg = (void *)HHI_VAPBCLK_CNTL,
	.bit_idx = 24,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_vapb_p1_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_vapb_p1_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_mux t5_vapb_mux = {
	.reg = (void *)HHI_VAPBCLK_CNTL,
	.mask = 0x1,
	.shift = 31,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vapb_mux",
		.ops = &clk_mux_ops,
		.parent_names = (const char *[]){ "t5_vapb_p0_gate",
			"t5_vapb_p1_gate"},
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_ge2d_gate = {
	.reg = (void *)HHI_VAPBCLK_CNTL,
	.bit_idx = 30,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_ge2d_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_vapb_mux" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const char * const t5_hdmirx_parent_names[] = { "xtal",
	"t5_fclk_div4", "t5_fclk_div3", "t5_fclk_div5" };

static struct clk_mux t5_hdmirx_cfg_mux = {
	.reg = (void *)HHI_HDMIRX_CLK_CNTL,
	.mask = 0x3,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hdmirx_cfg_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_hdmirx_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_hdmirx_cfg_div = {
	.reg = (void *)HHI_HDMIRX_CLK_CNTL,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hdmirx_cfg_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_hdmirx_cfg_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_hdmirx_cfg_gate = {
	.reg = (void *)HHI_HDMIRX_CLK_CNTL,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_hdmirx_cfg_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_hdmirx_cfg_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_mux t5_hdmirx_modet_mux = {
	.reg = (void *)HHI_HDMIRX_CLK_CNTL,
	.mask = 0x3,
	.shift = 25,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hdmirx_modet_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_hdmirx_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_hdmirx_modet_div = {
	.reg = (void *)HHI_HDMIRX_CLK_CNTL,
	.shift = 16,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hdmirx_modet_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_hdmirx_modet_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_hdmirx_modet_gate = {
	.reg = (void *)HHI_HDMIRX_CLK_CNTL,
	.bit_idx = 24,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_hdmirx_modet_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_hdmirx_modet_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static const char * const t5_hdmirx_acr_parent_names[] = { "t5_fclk_div4",
	"t5_fclk_div3", "t5_fclk_div5", "t5_fclk_div7" };

static struct clk_mux t5_hdmirx_acr_mux = {
	.reg = (void *)HHI_HDMIRX_AUD_CLK_CNTL,
	.mask = 0x3,
	.shift = 25,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hdmirx_acr_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_hdmirx_acr_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_hdmirx_acr_div = {
	.reg = (void *)HHI_HDMIRX_AUD_CLK_CNTL,
	.shift = 16,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hdmirx_acr_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_hdmirx_acr_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_hdmirx_acr_gate = {
	.reg = (void *)HHI_HDMIRX_AUD_CLK_CNTL,
	.bit_idx = 24,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_hdmirx_acr_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_hdmirx_acr_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static struct clk_mux t5_hdmirx_meter_mux = {
	.reg = (void *)HHI_HDMIRX_METER_CLK_CNTL,
	.mask = 0x3,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hdmirx_meter_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_hdmirx_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_hdmirx_meter_div = {
	.reg = (void *)HHI_HDMIRX_METER_CLK_CNTL,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hdmirx_meter_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_hdmirx_meter_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_hdmirx_meter_gate = {
	.reg = (void *)HHI_HDMIRX_METER_CLK_CNTL,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_hdmirx_meter_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_hdmirx_meter_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static const char * const t5_vdin_meas_parent_names[] = { "xtal", "t5_fclk_div4",
	"t5_fclk_div3", "t5_fclk_div5" };

static struct clk_mux t5_vdin_meas_mux = {
	.reg = (void *)HHI_VDIN_MEAS_CLK_CNTL,
	.mask = 0x7,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vdin_meas_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_vdin_meas_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_vdin_meas_div = {
	.reg = (void *)HHI_VDIN_MEAS_CLK_CNTL,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vdin_meas_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_vdin_meas_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_vdin_meas_gate = {
	.reg = (void *)HHI_VDIN_MEAS_CLK_CNTL,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_vdin_meas_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_vdin_meas_div" },
		.num_parents = 1,
		/* delete CLK_IGNORE_UNUSED in real chip */
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED,
	},
};

static const char * const sd_emmc_parent_names[] = { "xtal", "t5_fclk_div2",
	"t5_fclk_div3", "t5_fclk_div5", "t5_fclk_div2p5", "t5_mpll2", "t5_mpll3", "t5_gp0_pll" };

static struct clk_mux t5_sd_emmc_c_mux = {
	.reg = (void *)HHI_NAND_CLK_CNTL,
	.mask = 0x7,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_sd_emmc_mux_c",
		.ops = &clk_mux_ops,
		.parent_names = sd_emmc_parent_names,
		.num_parents = 8,
		.flags = (CLK_GET_RATE_NOCACHE),
	},
};

static struct clk_divider t5_sd_emmc_c_div = {
	.reg = (void *)HHI_NAND_CLK_CNTL,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_sd_emmc_div_c",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_sd_emmc_mux_c" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_sd_emmc_c_gate  = {
	.reg = (void *)HHI_NAND_CLK_CNTL,
	.bit_idx = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_sd_emmc_gate_c",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_sd_emmc_div_c" },
		.num_parents = 1,
		.flags = (CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED),
	},
};

static const char * const spicc_parent_names[] = { "xtal",
	"t5_clk81", "t5_fclk_div4", "t5_fclk_div3", "t5_fclk_div2", "t5_fclk_div5",
	"t5_fclk_div7"};

static struct clk_mux t5_spicc0_mux = {
	.reg = (void *)HHI_SPICC_CLK_CNTL,
	.mask = 0x7,
	.shift = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_spicc0_mux",
		.ops = &clk_mux_ops,
		.parent_names = spicc_parent_names,
		.num_parents = 7,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_spicc0_div = {
	.reg = (void *)HHI_SPICC_CLK_CNTL,
	.shift = 0,
	.width = 6,
	.lock = &clk_lock,
	.flags = CLK_DIVIDER_ROUND_CLOSEST,
	.hw.init = &(struct clk_init_data){
		.name = "t5_spicc0_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_spicc0_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_spicc0_gate = {
	.reg = (void *)HHI_SPICC_CLK_CNTL,
	.bit_idx = 6,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_spicc0_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_spicc0_div" },
		.num_parents = 1,
		.flags = (CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED),
	},
};

static struct clk_mux t5_vdac_clkc_mux = {
	.reg = (void *)HHI_CDAC_CLK_CNTL,
	.mask = 0x1,
	.shift = 16,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vdac_clkc_mux",
		.ops = &clk_mux_ops,
		.parent_names = (const char *[]){ "xtal", "t5_fclk_div5" },
		.num_parents = 2,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_vdac_clkc_div = {
	.reg = (void *)HHI_CDAC_CLK_CNTL,
	.shift = 0,
	.width = 16,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vdac_clkc_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_vdac_clkc_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_vdac_clkc_gate = {
	.reg = (void *)HHI_CDAC_CLK_CNTL,
	.bit_idx = 20,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_vdac_clkc_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_vdac_clkc_div" },
		.num_parents = 1,
		.flags = (CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT | CLK_IGNORE_UNUSED),
	},
};

/*
 * T5 : the last clk source is t5_fclk_div7
 * T5D: the last clk source is t5_fclk_div3.
 * And the last clk source is never used in T5, put t5_fclk_div3 here for T5D
 */
static const char * const t5_vpu_clkb_parent_names[] = { "t5_vpu_mux", "t5_fclk_div4",
				"t5_fclk_div5", "t5_fclk_div3" };

static struct clk_mux t5_vpu_clkb_tmp_mux = {
	.reg = (void *)HHI_VPU_CLKB_CNTL,
	.mask = 0x3,
	.shift = 20,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vpu_clkb_tmp_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_vpu_clkb_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_vpu_clkb_tmp_div = {
	.reg = (void *)HHI_VPU_CLKB_CNTL,
	.shift = 16,
	.width = 4,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vpu_clkb_tmp_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_vpu_clkb_tmp_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_vpu_clkb_tmp_gate = {
	.reg = (void *)HHI_VPU_CLKB_CNTL,
	.bit_idx = 24,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_vpu_clkb_tmp_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_vpu_clkb_tmp_div" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_divider t5_vpu_clkb_div = {
	.reg = (void *)HHI_VPU_CLKB_CNTL,
	.shift = 0,
	.width = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_vpu_clkb_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_vpu_clkb_tmp_gate" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_vpu_clkb_gate = {
	.reg = (void *)HHI_VPU_CLKB_CNTL,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_vpu_clkb_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_vpu_clkb_div" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static const char * const t5_tcon_pll_clk_parent_names[] = { "xtal", "t5_fclk_div5",
				"t5_fclk_div4", "t5_fclk_div3", "t5_mpll2", "t5_mpll3",
				"null", "null" };

static struct clk_mux t5_tcon_pll_clk_mux = {
	.reg = (void *)HHI_TCON_CLK_CNTL,
	.mask = 0x7,
	.shift = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_tcon_pll_clk_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_tcon_pll_clk_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_tcon_pll_clk_div = {
	.reg = (void *)HHI_TCON_CLK_CNTL,
	.shift = 0,
	.width = 6,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_tcon_pll_clk_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_tcon_pll_clk_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_tcon_pll_clk_gate = {
	.reg = (void *)HHI_TCON_CLK_CNTL,
	.bit_idx = 6,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_tcon_pll_clk_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_tcon_pll_clk_div" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_divider t5_vid_lock_div = {
	.reg = (void *)HHI_VID_LOCK_CLK_CNTL,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_vid_lock_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "xtal" },
		.num_parents = 1,
	},
};

static struct clk_gate t5_vid_lock_clk = {
	.reg = (void *)HHI_VID_LOCK_CLK_CNTL,
	.bit_idx = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_vid_lock_clk",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_vid_lock_div" },
		.num_parents = 1,
		.flags = CLK_SET_RATE_PARENT,
	},
};

static const char * const t5_hdmi_axi_parent_names[] = { "xtal", "t5_fclk_div4",
				"t5_fclk_div3", "t5_fclk_div5"};

static struct clk_mux t5_hdmi_axi_mux = {
	.reg = (void *)HHI_HDMI_AXI_CLK_CNTL,
	.mask = 0x3,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hdmi_axi_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_hdmi_axi_parent_names,
		.num_parents = 4,
	},
};

static struct clk_divider t5_hdmi_axi_div = {
	.reg = (void *)HHI_HDMI_AXI_CLK_CNTL,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_hdmi_axi_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_hdmi_axi_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_hdmi_axi_gate = {
	.reg = (void *)HHI_HDMI_AXI_CLK_CNTL,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_hdmi_axi_gate",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_hdmi_axi_div" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

/*t5d newly */
static const char * const t5_demod_t2_parent_names[] = { "xtal",
	"t5_fclk_div5", "t5_fclk_div4", "t5_adc_extclk_in" };

static struct clk_mux t5_demod_core_t2_mux = {
	.reg = (void *)HHI_DEMOD_CLK_CNTL1,
	.mask = 0x3,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_demod_core_t2_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_demod_t2_parent_names,
		.num_parents = 4,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_demod_core_t2_div = {
	.reg = (void *)HHI_DEMOD_CLK_CNTL1,
	.shift = 0,
	.width = 7,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_demod_core_t2_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_demod_core_t2_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_demod_core_t2 = {
	.reg = (void *)HHI_DEMOD_CLK_CNTL1,
	.bit_idx = 8,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_demod_core_t2",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_demod_core_t2_div" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static const char * const t5_tsin_parent_names[] = { "t5_fclk_div2",
	"xtal", "t5_fclk_div4", "t5_fclk_div3", "t5_fclk_div2p5",
	"t5_fclk_div7", "" };

static struct clk_mux t5_tsin_deglich_mux = {
	.reg = (void *)HHI_TSIN_DEGLITCH_CLK_CNTL,
	.mask = 0x3,
	.shift = 9,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_tsin_deglich_mux",
		.ops = &clk_mux_ops,
		.parent_names = t5_tsin_parent_names,
		.num_parents = 8,
		.flags = CLK_GET_RATE_NOCACHE,
	},
};

static struct clk_divider t5_tsin_deglich_div = {
	.reg = (void *)HHI_TSIN_DEGLITCH_CLK_CNTL,
	.shift = 0,
	.width = 6,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data){
		.name = "t5_tsin_deglich_div",
		.ops = &clk_divider_ops,
		.parent_names = (const char *[]){ "t5_tsin_deglich_mux" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_gate t5_tsin_deglich = {
	.reg = (void *)HHI_TSIN_DEGLITCH_CLK_CNTL,
	.bit_idx = 6,
	.lock = &clk_lock,
	.hw.init = &(struct clk_init_data) {
		.name = "t5_tsin_deglich",
		.ops = &clk_gate_ops,
		.parent_names = (const char *[]){ "t5_tsin_deglich_div" },
		.num_parents = 1,
		.flags = CLK_GET_RATE_NOCACHE | CLK_SET_RATE_PARENT,
	},
};

static struct clk_mux *t5_per_clk_muxs[] = {
	&t5_mpeg_clk_sel,
	&t5_gpu_p0_mux,
	&t5_gpu_p1_mux,
	&t5_gpu_mux,
	&t5_vpu_p0_mux,
	&t5_vpu_p1_mux,
	&t5_vpu_mux,
	&t5_vpu_clkc_p0_mux,
	&t5_vpu_clkc_p1_mux,
	&t5_vpu_clkc_mux,
	&t5_adc_extclk_in_mux,
	&t5_demod_core_clk_mux,
	&t5_vdec_p0_mux,
	&t5_hevcf_p0_mux,
	&t5_vdec_p1_mux,
	&t5_vdec_mux,
	&t5_hevcf_p1_mux,
	&t5_hevcf_mux,
	&t5_hdcp22_esm_mux,
	&t5_hdcp22_skp_mux,
	&t5_vapb_p0_mux,
	&t5_vapb_p1_mux,
	&t5_vapb_mux,
	&t5_hdmirx_cfg_mux,
	&t5_hdmirx_modet_mux,
	&t5_hdmirx_acr_mux,
	&t5_hdmirx_meter_mux,
	&t5_vdin_meas_mux,
	&t5_sd_emmc_c_mux,
	&t5_spicc0_mux,
	&t5_vdac_clkc_mux,
	&t5_vpu_clkb_tmp_mux,
	&t5_tcon_pll_clk_mux,
	&t5_hdmi_axi_mux,
	&t5_demod_core_t2_mux,
	&t5_tsin_deglich_mux
};

static struct clk_divider *t5_per_clk_divs[] = {
	&t5_mpeg_clk_div,
	&t5_ts_div,
	&t5_gpu_p0_div,
	&t5_gpu_p1_div,
	&t5_vpu_p0_div,
	&t5_vpu_p1_div,
	&t5_vpu_clkc_p0_div,
	&t5_vpu_clkc_p1_div,
	&t5_adc_extclk_in_div,
	&t5_demod_core_clk_div,
	&t5_vdec_p0_div,
	&t5_hevcf_p0_div,
	&t5_vdec_p1_div,
	&t5_hevcf_p1_div,
	&t5_hdcp22_esm_div,
	&t5_hdcp22_skp_div,
	&t5_vapb_p0_div,
	&t5_vapb_p1_div,
	&t5_hdmirx_cfg_div,
	&t5_hdmirx_modet_div,
	&t5_hdmirx_acr_div,
	&t5_hdmirx_meter_div,
	&t5_vdin_meas_div,
	&t5_sd_emmc_c_div,
	&t5_spicc0_div,
	&t5_vdac_clkc_div,
	&t5_vpu_clkb_tmp_div,
	&t5_vpu_clkb_div,
	&t5_tcon_pll_clk_div,
	&t5_vid_lock_div,
	&t5_hdmi_axi_div,
	&t5_demod_core_t2_div,
	&t5_tsin_deglich_div
};

static struct clk_gate *t5_per_clk_gates[] = {
	&t5_clk81,
	&t5_ts,
	&t5_gpu_p0_gate,
	&t5_gpu_p1_gate,
	&t5_vpu_p0_gate,
	&t5_vpu_p1_gate,
	&t5_vpu_clkc_p0_gate,
	&t5_vpu_clkc_p1_gate,
	&t5_adc_extclk_in,
	&t5_demod_core_clk,
	&t5_vdec_p0_gate,
	&t5_hevcf_p0_gate,
	&t5_vdec_p1_gate,
	&t5_hevcf_p1_gate,
	&t5_hdcp22_esm_gate,
	&t5_hdcp22_skp_gate,
	&t5_vapb_p0_gate,
	&t5_vapb_p1_gate,
	&t5_hdmirx_cfg_gate,
	&t5_hdmirx_modet_gate,
	&t5_hdmirx_acr_gate,
	&t5_hdmirx_meter_gate,
	&t5_vdin_meas_gate,
	&t5_sd_emmc_c_gate,
	&t5_spicc0_gate,
	&t5_vdac_clkc_gate,
	&t5_ge2d_gate,
	&t5_vpu_clkb_tmp_gate,
	&t5_vpu_clkb_gate,
	&t5_tcon_pll_clk_gate,
	&t5_vid_lock_clk,
	&t5_hdmi_axi_gate,
	&t5_demod_core_t2,
	&t5_tsin_deglich,
};

/* Array of all clocks provided by this provider */
static struct clk_hw *t5_per_clk_hws[NR_PER_CLKS] = {
	[CLKID_MPEG_SEL]	= &t5_mpeg_clk_sel.hw,
	[CLKID_MPEG_DIV]	= &t5_mpeg_clk_div.hw,
	[CLKID_CLK81]		= &t5_clk81.hw,
	[CLKID_TS_DIV]		= &t5_ts_div.hw,
	[CLKID_TS]		= &t5_ts.hw,
	[CLKID_GPU_P0_MUX]	= &t5_gpu_p0_mux.hw,
	[CLKID_GPU_P0_DIV]	= &t5_gpu_p0_div.hw,
	[CLKID_GPU_P0_GATE]	= &t5_gpu_p0_gate.hw,
	[CLKID_GPU_P1_MUX]	= &t5_gpu_p1_mux.hw,
	[CLKID_GPU_P1_DIV]	= &t5_gpu_p1_div.hw,
	[CLKID_GPU_P1_GATE]	= &t5_gpu_p1_gate.hw,
	[CLKID_GPU_MUX]		= &t5_gpu_mux.hw,
	[CLKID_VPU_P0_MUX]	= &t5_vpu_p0_mux.hw,
	[CLKID_VPU_P0_DIV]	= &t5_vpu_p0_div.hw,
	[CLKID_VPU_P0_GATE]	= &t5_vpu_p0_gate.hw,
	[CLKID_VPU_P1_MUX]	= &t5_vpu_p1_mux.hw,
	[CLKID_VPU_P1_DIV]	= &t5_vpu_p1_div.hw,
	[CLKID_VPU_P1_GATE]	= &t5_vpu_p1_gate.hw,
	[CLKID_VPU_MUX]		= &t5_vpu_mux.hw,
	[CLKID_VPU_CLKC_P0_MUX]	= &t5_vpu_clkc_p0_mux.hw,
	[CLKID_VPU_CLKC_P0_DIV]	= &t5_vpu_clkc_p0_div.hw,
	[CLKID_VPU_CLKC_P0_GATE]	= &t5_vpu_clkc_p0_gate.hw,
	[CLKID_VPU_CLKC_P1_MUX]	= &t5_vpu_clkc_p1_mux.hw,
	[CLKID_VPU_CLKC_P1_DIV]	= &t5_vpu_clkc_p1_div.hw,
	[CLKID_VPU_CLKC_P1_GATE]	= &t5_vpu_clkc_p1_gate.hw,
	[CLKID_VPU_CLKC_MUX]	= &t5_vpu_clkc_mux.hw,
	[CLKID_ADC_EXTCLK_IN_MUX]	= &t5_adc_extclk_in_mux.hw,
	[CLKID_ADC_EXTCLK_IN_DIV]	= &t5_adc_extclk_in_div.hw,
	[CLKID_ADC_EXTCLK_IN]		= &t5_adc_extclk_in.hw,
	[CLKID_DEMOOD_CORE_CLK_MUX]	= &t5_demod_core_clk_mux.hw,
	[CLKID_DEMOOD_CORE_CLK_DIV]	= &t5_demod_core_clk_div.hw,
	[CLKID_DEMOOD_CORE_CLK]		= &t5_demod_core_clk.hw,
	[CLKID_VDEC_P0_MUX]		= &t5_vdec_p0_mux.hw,
	[CLKID_VDEC_P0_DIV]		= &t5_vdec_p0_div.hw,
	[CLKID_VDEC_P0_GATE]		= &t5_vdec_p0_gate.hw,
	[CLKID_HEVCF_P0_MUX]		= &t5_hevcf_p0_mux.hw,
	[CLKID_HEVCF_P0_DIV]		= &t5_hevcf_p0_div.hw,
	[CLKID_HEVCF_P0_GATE]		= &t5_hevcf_p0_gate.hw,
	[CLKID_VDEC_P1_MUX]		= &t5_vdec_p1_mux.hw,
	[CLKID_VDEC_P1_DIV]		= &t5_vdec_p1_div.hw,
	[CLKID_VDEC_P1_GATE]		= &t5_vdec_p1_gate.hw,
	[CLKID_VDEC_MUX]		= &t5_vdec_mux.hw,
	[CLKID_HEVCF_P1_MUX]		= &t5_hevcf_p1_mux.hw,
	[CLKID_HEVCF_P1_DIV]		= &t5_hevcf_p1_div.hw,
	[CLKID_HEVCF_P1_GATE]		= &t5_hevcf_p1_gate.hw,
	[CLKID_HEVCF_MUX]		= &t5_hevcf_mux.hw,
	[CLKID_HDCP22_ESM_MUX]		= &t5_hdcp22_esm_mux.hw,
	[CLKID_HDCP22_SKP_MUX]		= &t5_hdcp22_skp_mux.hw,
	[CLKID_HDCP22_ESM_DIV]		= &t5_hdcp22_esm_div.hw,
	[CLKID_HDCP22_SKP_DIV]		= &t5_hdcp22_skp_div.hw,
	[CLKID_HDCP22_ESM_GATE]		= &t5_hdcp22_esm_gate.hw,
	[CLKID_HDCP22_SKP_GATE]		= &t5_hdcp22_skp_gate.hw,
	[CLKID_VAPB_P0_MUX]		= &t5_vapb_p0_mux.hw,
	[CLKID_VAPB_P1_MUX]		= &t5_vapb_p1_mux.hw,
	[CLKID_VAPB_P0_DIV]		= &t5_vapb_p0_div.hw,
	[CLKID_VAPB_P1_DIV]		= &t5_vapb_p1_div.hw,
	[CLKID_VAPB_P0_GATE]		= &t5_vapb_p0_gate.hw,
	[CLKID_VAPB_P1_GATE]		= &t5_vapb_p1_gate.hw,
	[CLKID_VAPB_MUX]		= &t5_vapb_mux.hw,
	[CLKID_HDMIRX_CFG_MUX]		= &t5_hdmirx_cfg_mux.hw,
	[CLKID_HDMIRX_MODET_MUX]	= &t5_hdmirx_modet_mux.hw,
	[CLKID_HDMIRX_CFG_DIV]		= &t5_hdmirx_cfg_div.hw,
	[CLKID_HDMIRX_MODET_DIV]	= &t5_hdmirx_modet_div.hw,
	[CLKID_HDMIRX_CFG_GATE]		= &t5_hdmirx_cfg_gate.hw,
	[CLKID_HDMIRX_MODET_GATE]	= &t5_hdmirx_modet_gate.hw,
	[CLKID_HDMIRX_ACR_MUX]		= &t5_hdmirx_acr_mux.hw,
	[CLKID_HDMIRX_ACR_DIV]		= &t5_hdmirx_acr_div.hw,
	[CLKID_HDMIRX_ACR_GATE]		= &t5_hdmirx_acr_gate.hw,
	[CLKID_HDMIRX_METER_MUX]	= &t5_hdmirx_meter_mux.hw,
	[CLKID_HDMIRX_METER_DIV]	= &t5_hdmirx_meter_div.hw,
	[CLKID_HDMIRX_METER_GATE]	= &t5_hdmirx_meter_gate.hw,
	[CLKID_VDIN_MEAS_MUX]		= &t5_vdin_meas_mux.hw,
	[CLKID_VDIN_MEAS_DIV]		= &t5_vdin_meas_div.hw,
	[CLKID_VDIN_MEAS_GATE]		= &t5_vdin_meas_gate.hw,
	[CLKID_SD_EMMC_C_MUX]		= &t5_sd_emmc_c_mux.hw,
	[CLKID_SD_EMMC_C_DIV]		= &t5_sd_emmc_c_div.hw,
	[CLKID_SD_EMMC_C_GATE]		= &t5_sd_emmc_c_gate.hw,
	[CLKID_SPICC0_MUX]		= &t5_spicc0_mux.hw,
	[CLKID_SPICC0_DIV]		= &t5_spicc0_div.hw,
	[CLKID_SPICC0_GATE]		= &t5_spicc0_gate.hw,
	[CLKID_VDAC_CLKC_MUX]		= &t5_vdac_clkc_mux.hw,
	[CLKID_VDAC_CLKC_DIV]		= &t5_vdac_clkc_div.hw,
	[CLKID_VDAC_CLKC_GATE]		= &t5_vdac_clkc_gate.hw,
	[CLKID_GE2D_GATE]               = &t5_ge2d_gate.hw,
	[CLKID_VPU_CLKB_TMP_MUX]	= &t5_vpu_clkb_tmp_mux.hw,
	[CLKID_VPU_CLKB_TMP_DIV]	= &t5_vpu_clkb_tmp_div.hw,
	[CLKID_VPU_CLKB_TMP_GATE]	= &t5_vpu_clkb_tmp_gate.hw,
	[CLKID_VPU_CLKB_DIV]		= &t5_vpu_clkb_div.hw,
	[CLKID_VPU_CLKB_GATE]		= &t5_vpu_clkb_gate.hw,
	[CLKID_TCON_PLL_CLK_MUX]	= &t5_tcon_pll_clk_mux.hw,
	[CLKID_TCON_PLL_CLK_DIV]	= &t5_tcon_pll_clk_div.hw,
	[CLKID_TCON_PLL_CLK_GATE]	= &t5_tcon_pll_clk_gate.hw,
	[CLKID_VID_LOCK_DIV]		= &t5_vid_lock_div.hw,
	[CLKID_VID_LOCK_CLK]		= &t5_vid_lock_clk.hw,
	[CLKID_HDMI_AXI_MUX]		= &t5_hdmi_axi_mux.hw,
	[CLKID_HDMI_AXI_DIV]		= &t5_hdmi_axi_div.hw,
	[CLKID_HDMI_AXI_GATE]		= &t5_hdmi_axi_gate.hw,
	[CLKID_DEMOD_T2_MUX]		= &t5_demod_core_t2_mux.hw,
	[CLKID_DEMOD_T2_DIV]		= &t5_demod_core_t2_div.hw,
	[CLKID_DEMOD_T2_GATE]		= &t5_demod_core_t2.hw,
	[CLKID_TSIN_DEGLICH_MUX]	= &t5_tsin_deglich_mux.hw,
	[CLKID_TSIN_DEGLICH_DIV]	= &t5_tsin_deglich_div.hw,
	[CLKID_TSIN_DEGLICH_GATE]	= &t5_tsin_deglich.hw,
};

static struct clk_onecell_data peri_clk_data;

static void __init t5_peripheral_clkc_init(struct device_node *np)
{
	int  i, ret;
	void __iomem *base;
	struct clk **peri_clks;

	base = of_iomap(np, 0);
	if (!base)
		return;

	peri_clks = kcalloc(NR_PER_CLKS, sizeof(struct clk *), GFP_KERNEL);
	peri_clk_data.clks = peri_clks;
	peri_clk_data.clk_num = NR_PER_CLKS;

	/* Populate base address for gates */
	for (i = 0; i < ARRAY_SIZE(t5_per_clk_gates); i++)
		t5_per_clk_gates[i]->reg = base +
			(unsigned long)t5_per_clk_gates[i]->reg;

	/* Populate base address for dividers  */
	for (i = 0; i < ARRAY_SIZE(t5_per_clk_divs); i++)
		t5_per_clk_divs[i]->reg = base +
			(unsigned long)t5_per_clk_divs[i]->reg;

	/* Populate base address for muxs  */
	for (i = 0; i < ARRAY_SIZE(t5_per_clk_muxs); i++)
		t5_per_clk_muxs[i]->reg = base +
			(unsigned long)t5_per_clk_muxs[i]->reg;

	peri_clks[CLKID_SD_EMMC_C_P0_COMP] = clk_register_composite(NULL,
		"sd_sd_emmc_c_comp", sd_emmc_parent_names, 8,
		&t5_sd_emmc_c_mux.hw, &clk_mux_ops,
		&t5_sd_emmc_c_div.hw, &clk_divider_ops,
		&t5_sd_emmc_c_gate.hw, &clk_gate_ops, CLK_IGNORE_UNUSED);

	for (i = 0; i < ARRAY_SIZE(t5_per_clk_hws); i++) {
		if (t5_per_clk_hws[i])
			peri_clks[i] = clk_register(NULL, t5_per_clk_hws[i]);
	}

	ret = of_clk_add_provider(np, of_clk_src_onecell_get, &peri_clk_data);
	if (ret)
		pr_err("add peripheral clk provider failed\n");
}

CLK_OF_DECLARE(t5, "amlogic,t5-peripheral-clkc", t5_peripheral_clkc_init);

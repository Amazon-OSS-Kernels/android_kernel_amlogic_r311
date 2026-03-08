/*
 * sound/soc/aml/m8/aml_g9tv.h
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
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

#ifndef AML_G9TV_H
#define AML_G9TV_H

#include <sound/soc.h>
#include <linux/gpio/consumer.h>

#define AML_I2C_BUS_AO 0
#define AML_I2C_BUS_A 1
#define AML_I2C_BUS_B 2
#define AML_I2C_BUS_C 3
#define AML_I2C_BUS_D 4

#define ANDROID_HP_SWITCH

enum HDMIIN_format {
	REFER_TO_HEADER = 0,
	LPCM = 1,
	AC3,
	MPEG1,
	MP3,
	MPEG2,
	AAC,
	DTS,
	ATRAC,
	ONE_BIT_AUDIO,
	DDP,
	DTS_HD,
	MAT,
	DST,
	WMA_PRO
};

struct aml_audio_private_data {
	bool suspended;
	void *data;

	int hp_last_state;
	int db_on_last_status;
	bool hp_det_status;
	int av_hs_switch;
	int hp_det_inv;
	int timer_en;
	int detect_flag;
	int db_on_det_flag;
	struct work_struct work;
	struct mutex lock;
	struct gpio_desc *hp_det_desc;
#ifdef ANDROID_HP_SWITCH
	struct switch_dev sdev;
#endif
	struct gpio_desc *db_on_det_desc;

	struct pinctrl *pin_ctl;
	struct timer_list timer;
	struct gpio_desc *av_mute_desc;
	int av_mute_inv;
	struct gpio_desc *amp_mute_desc;
	int amp_mute_inv;
	struct clk *clk;
	int sleep_time;
	struct work_struct pinmux_work;
#ifdef CONFIG_AML_AO_CEC
	int arc_enable;
#endif
#ifdef CONFIG_TVIN_HDMI
	int atmos_edid_enable;
#endif
	enum HDMIIN_format hal_fmt;
	struct switch_dev hal_fmt_sdev;
};

struct aml_audio_codec_info {
	const char *name;
	const char *status;
	struct device_node *p_node;
	unsigned i2c_bus_type;
	unsigned i2c_addr;
	unsigned id_reg;
	unsigned id_val;
	unsigned capless;
};

struct codec_info {
	char name[I2C_NAME_SIZE];
	char name_bus[I2C_NAME_SIZE];
};

struct codec_probe_priv {
	int num_eq;
	struct tas57xx_eq_cfg *eq_configs;
};

static int aml_get_eqdrc_reg(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol);
static int aml_set_eqdrc_reg(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol);

extern int External_Mute(int mute_flag);
struct rx_audio_stat_s;
extern void rx_get_audio_status(struct rx_audio_stat_s *aud_sts);
extern void rx_set_atmos_flag(bool en);
extern void rx_get_atmos_flag(void);
extern void aml_fe_get_atvaudio_state(int *state);
extern int tvin_get_av_status(void);
#endif

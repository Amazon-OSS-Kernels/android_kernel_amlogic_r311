/*
 * linux/sound/soc/codecs/tlv320dac3203.c
 *
 * Copyright 2011 Amlogic
 * Author: Xing Fang <xing.fang@amlogic.com>
 * Based on sound/soc/codecs/tlv320aic32x4 and
 * TI driver for kernel 2.6.27.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>

#include <sound/tlv320aic32x4.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include "tlv320dac3203.h"

struct dac3203_configs {
	u32 reg_offset;
	u8 reg_val;
};

static const struct dac3203_configs biquad_Play[] = {
	/* Playback Filters */
	/* Force DRC (Dynamic Range Compression) off */
	{DAC3203_DRCCTRLREG1, 0x0F},
};

struct dac3203_priv {
	struct regmap *regmap;
	struct i2c_client *dev;
	u32 sysclk;
	u32 power_cfg;
	u32 micpga_routing;
	bool swapdacs;
	int rstn_gpio;
	int amp_enable;
	struct clk *mclk;
	int channels;
	u16 unmuted;
	bool ignore_ramp;
	u8  page_no; // added from original code, might conflict with regmap

	struct regulator *supply_ldo;
	struct regulator *supply_iov;
	struct regulator *supply_dv;
	struct regulator *supply_av;
};

static DEFINE_MUTEX(i2c_access);

/* 0dB min, 0.5dB steps */
static DECLARE_TLV_DB_SCALE(tlv_step_0_5, 0, 50, 0);
/* -63.5dB min, 0.5dB steps */
static DECLARE_TLV_DB_SCALE(tlv_pcm, -6350, 50, 0);
/* -6dB min, 1dB steps */
static DECLARE_TLV_DB_SCALE(tlv_driver_gain, -600, 100, 0);
/* -12dB min, 0.5dB steps */
static DECLARE_TLV_DB_SCALE(tlv_adc_vol, -1200, 50, 0);

static const char * const drc_enable[] = { "Disabled", "Enabled" };
static const struct soc_enum dac3203_drc_ctrl_reg1 =
		SOC_ENUM_DOUBLE(DAC3203_DRCCTRLREG1, 6, 5, 2, drc_enable);

static const char * const control_enable[] = { "Off", "On" };

static const char * const dac_soft_stepping_control[] = {
	"1 step/sample", "1 step/2 sample", "disabled"
};
static const struct soc_enum dac3203_dac_soft_stepping_ctrl =
	SOC_ENUM_SINGLE(DAC3203_DACSETUP, 0, 3, dac_soft_stepping_control);
static const struct soc_enum dac3203_mono_enable_enum =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(control_enable), control_enable);
static const struct soc_enum dac3203_ramp_enable_enum =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(control_enable), control_enable);

static u8 dac_soft_stepping = DAC3203_DAC_DEFAULT_SOFT_STEPPING;

static u8 need_pll_on;

static struct dac3203_configs biquad_settings_regs[] = {
	/* Playback Filters */
	{DAC3203_PAGE44 + 12, 0},
	{DAC3203_PAGE44 + 13, 0},
	{DAC3203_PAGE44 + 14, 0},
	{DAC3203_PAGE44 + 16, 0},
	{DAC3203_PAGE44 + 17, 0},
	{DAC3203_PAGE44 + 18, 0},
	{DAC3203_PAGE44 + 20, 0},
	{DAC3203_PAGE44 + 21, 0},
	{DAC3203_PAGE44 + 22, 0},
	{DAC3203_PAGE44 + 24, 0},
	{DAC3203_PAGE44 + 25, 0},
	{DAC3203_PAGE44 + 26, 0},
	{DAC3203_PAGE44 + 28, 0},
	{DAC3203_PAGE44 + 29, 0},
	{DAC3203_PAGE44 + 30, 0},
	{DAC3203_PAGE44 + 32, 0},
	{DAC3203_PAGE44 + 33, 0},
	{DAC3203_PAGE44 + 34, 0},
	{DAC3203_PAGE44 + 36, 0},
	{DAC3203_PAGE44 + 37, 0},
	{DAC3203_PAGE44 + 38, 0},
	{DAC3203_PAGE44 + 40, 0},
	{DAC3203_PAGE44 + 41, 0},
	{DAC3203_PAGE44 + 42, 0},
	{DAC3203_PAGE44 + 44, 0},
	{DAC3203_PAGE44 + 45, 0},
	{DAC3203_PAGE44 + 46, 0},
	{DAC3203_PAGE44 + 48, 0},
	{DAC3203_PAGE44 + 49, 0},
	{DAC3203_PAGE44 + 50, 0},
	{DAC3203_PAGE44 + 52, 0},
	{DAC3203_PAGE44 + 53, 0},
	{DAC3203_PAGE44 + 54, 0},
	{DAC3203_PAGE44 + 56, 0},
	{DAC3203_PAGE44 + 57, 0},
	{DAC3203_PAGE44 + 58, 0},
	{DAC3203_PAGE44 + 60, 0},
	{DAC3203_PAGE44 + 61, 0},
	{DAC3203_PAGE44 + 62, 0},
	{DAC3203_PAGE44 + 64, 0},
	{DAC3203_PAGE44 + 65, 0},
	{DAC3203_PAGE44 + 66, 0},
	{DAC3203_PAGE44 + 68, 0},
	{DAC3203_PAGE44 + 69, 0},
	{DAC3203_PAGE44 + 70, 0},
	{DAC3203_PAGE45 + 20, 0},
	{DAC3203_PAGE45 + 21, 0},
	{DAC3203_PAGE45 + 22, 0},
	{DAC3203_PAGE45 + 24, 0},
	{DAC3203_PAGE45 + 25, 0},
	{DAC3203_PAGE45 + 26, 0},
	{DAC3203_PAGE45 + 28, 0},
	{DAC3203_PAGE45 + 29, 0},
	{DAC3203_PAGE45 + 30, 0},
	{DAC3203_PAGE45 + 32, 0},
	{DAC3203_PAGE45 + 33, 0},
	{DAC3203_PAGE45 + 34, 0},
	{DAC3203_PAGE45 + 36, 0},
	{DAC3203_PAGE45 + 37, 0},
	{DAC3203_PAGE45 + 38, 0},
	{DAC3203_PAGE45 + 40, 0},
	{DAC3203_PAGE45 + 41, 0},
	{DAC3203_PAGE45 + 42, 0},
	{DAC3203_PAGE45 + 44, 0},
	{DAC3203_PAGE45 + 45, 0},
	{DAC3203_PAGE45 + 46, 0},
	{DAC3203_PAGE45 + 48, 0},
	{DAC3203_PAGE45 + 49, 0},
	{DAC3203_PAGE45 + 50, 0},
	{DAC3203_PAGE45 + 52, 0},
	{DAC3203_PAGE45 + 53, 0},
	{DAC3203_PAGE45 + 54, 0},
	{DAC3203_PAGE45 + 56, 0},
	{DAC3203_PAGE45 + 57, 0},
	{DAC3203_PAGE45 + 58, 0},
	{DAC3203_PAGE45 + 60, 0},
	{DAC3203_PAGE45 + 61, 0},
	{DAC3203_PAGE45 + 62, 0},
	{DAC3203_PAGE45 + 64, 0},
	{DAC3203_PAGE45 + 65, 0},
	{DAC3203_PAGE45 + 66, 0},
	{DAC3203_PAGE45 + 68, 0},
	{DAC3203_PAGE45 + 69, 0},
	{DAC3203_PAGE45 + 70, 0},
	{DAC3203_PAGE45 + 72, 0},
	{DAC3203_PAGE45 + 73, 0},
	{DAC3203_PAGE45 + 74, 0},
	{DAC3203_PAGE45 + 76, 0},
	{DAC3203_PAGE45 + 77, 0},
	{DAC3203_PAGE45 + 78, 0},
	{DAC3203_PAGE46 + 52, 0},    /* DRC HPF: Page 46: N0 52 - 55 */
	{DAC3203_PAGE46 + 53, 0},    /* Bypassed: N0 = 0x7FFFFF */
	{DAC3203_PAGE46 + 54, 0},
	{DAC3203_PAGE46 + 55, 0},    /* N1 = 0 */
	{DAC3203_PAGE46 + 56, 0},
	{DAC3203_PAGE46 + 57, 0},
	{DAC3203_PAGE46 + 58, 0},
	{DAC3203_PAGE46 + 59, 0},
	{DAC3203_PAGE46 + 60, 0},   /* D1 */
	{DAC3203_PAGE46 + 61, 0},
	{DAC3203_PAGE46 + 62, 0},
	{DAC3203_PAGE46 + 63, 0},
	{DAC3203_PAGE46 + 64, 0},    /* DRC LPF: Page 46: N0 64 - 67 */
	{DAC3203_PAGE46 + 65, 0},    /* Bypassed: N0 = 0x7FFFFF */
	{DAC3203_PAGE46 + 66, 0},
	{DAC3203_PAGE46 + 67, 0},    /* N1 = 0 */
	{DAC3203_PAGE46 + 68, 0},
	{DAC3203_PAGE46 + 69, 0},
	{DAC3203_PAGE46 + 70, 0},
	{DAC3203_PAGE46 + 71, 0},
	{DAC3203_PAGE46 + 72, 0},   /* D1 */
	{DAC3203_PAGE46 + 73, 0},
	{DAC3203_PAGE46 + 74, 0},
	{DAC3203_PAGE46 + 75, 0},
#ifdef SET_DRC_OFF
	/* Force DRC (Dynamic Range Compression) off */
	{DAC3203_DRCCTRLREG1, 0x0},
#else
	/* Enable DRC */
	{DAC3203_DRCCTRLREG1, 0x0},
	{DAC3203_DRCCTRLREG2, 0x0},
	{DAC3203_DRCCTRLREG3, 0x0}
#endif
};

#define BIQUAD_SETTINGS_REGS_SIZE \
	(sizeof(biquad_settings_regs)/sizeof(struct dac3203_configs))

static bool dac3203_volatile(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case DAC3203_PSEL: /* regmap implementation requires this */
	case DAC3203_RESET: /* always clears after write */
	case DAC3203_GAIN_APPLIED:
		return true;
	}
	return false;
}

static int dac3203_biquad_coeff_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct soc_bytes_ext *bytes_ext =
		(struct soc_bytes_ext *) kcontrol->private_value;
	int i;

	if (bytes_ext->max != BIQUAD_SETTINGS_REGS_SIZE)
		return -EINVAL;

	for (i = 0; i < BIQUAD_SETTINGS_REGS_SIZE; i++) {
		ucontrol->value.bytes.data[i] =
			biquad_settings_regs[i].reg_val;
	}

	return 0;
}

static int dac3203_biquad_coeff_set(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	struct soc_bytes_ext *bytes_ext =
			(struct soc_bytes_ext *) kcontrol->private_value;
	int i;

	if (bytes_ext->max != BIQUAD_SETTINGS_REGS_SIZE)
		return -EINVAL;

	for (i = 0; i < BIQUAD_SETTINGS_REGS_SIZE; i++) {
		biquad_settings_regs[i].reg_val = ucontrol->value.bytes.data[i];
		snd_soc_write(codec,
			biquad_settings_regs[i].reg_offset,
						ucontrol->value.bytes.data[i]);
	}

	return 0;
}

static int snd_soc_put_enum_double_wrapper(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct soc_enum *e = (struct soc_enum *)kcontrol->private_value;
	unsigned int *item = ucontrol->value.enumerated.item;

	/* struct soc_enum doesn't have the member of max in 4.9 kernel */
#if 0
	if (item[0] >= e->max)
		return -EINVAL;
#endif

	dac_soft_stepping =
		snd_soc_enum_item_to_val(e, item[0]) << e->shift_l;

	return snd_soc_put_enum_double(kcontrol, ucontrol);
}

static int dac3203_apply_mute(struct snd_soc_codec *codec, int mute)
{
	struct dac3203_priv *dac3203 = snd_soc_codec_get_drvdata(codec);
	u8 unmute_state, prog_gain, ref_gain;
	int i = 0;

	dev_info(codec->dev, "%s+ mute=%d\n", __func__, mute);

	/* allow DAC gain ramp up at boot time only
	 * once it is unmuted, ignore any request mute/unmute
	 * on pcm_open/pcm_close on I2S device node for pcm
	 * playback.
	 */

	/* disable this change till TI/HW team has path to avoid
	 * initial audio cut at playback
	 */
	if (dac3203->unmuted == 1)
		return 0;

	unmute_state = snd_soc_read(codec, DAC3203_DACMUTE) & ~DAC3203_MUTEON;
	prog_gain = snd_soc_read(codec, DAC3203_GAIN_APPLIED);

	if (mute)
		snd_soc_write(codec, DAC3203_DACMUTE,
			unmute_state | DAC3203_MUTEON);
	else {
		if (dac3203->channels == 1) {
			/* For mono, check HP Driver only for one channel
			 * and unmute one channel too
			 */
			unmute_state |= DAC3203_UNMUTE_MONO;
			ref_gain = DAC_GAIN_MONO_APPLIED;
		} else {
			ref_gain = DAC_GAIN_STEREO_APPLIED;
		}

		if (dac3203->ignore_ramp == false) {
			/* Before unmuting wait for DAC gain to reach applied
			 * level. Not doing so will cause a pop. It can take
			 * upto 2sec. Limit the wait by a counter.
			 */
			while (prog_gain < ref_gain && i < 22) {
				/* wait for applied gain to reduce Pop */
				prog_gain = snd_soc_read(codec,
						DAC3203_GAIN_APPLIED);
				msleep(100);
				i++;
			}
		}

		/* Unnmute required number of channels */
		snd_soc_write(codec, DAC3203_DACMUTE, unmute_state);
	}
	/* Store device current status */
	dac3203->unmuted = !mute;

	dev_info(codec->dev, "%s- mute=%d prog_gain=%x i=%d\n", __func__, mute,
		prog_gain, i);

	return 0;
}

static int put_right_ch_enab_only(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	struct dac3203_priv *dac3203 = snd_soc_codec_get_drvdata(codec);
	int prev_channels = dac3203->channels;

	dev_info(codec->dev, "%s right_only=%d, current_ch=%d unmute=%d\n",
		__func__, ucontrol->value.enumerated.item[0], dac3203->channels,
		dac3203->unmuted);

	if (ucontrol->value.enumerated.item[0] >
		ARRAY_SIZE(control_enable)) {
		pr_err("%s: Mono Channel Settings Invalid value=%d\n", __func__,
			ucontrol->value.enumerated.item[0]);
		return -EINVAL;
	}

	if (ucontrol->value.enumerated.item[0])
		dac3203->channels = 1;
	else
		dac3203->channels = 2;

	/* If channel count was updated and DAC is in unmute state,
	 * apply unmute to correct number of channels
	 */
	if (prev_channels != dac3203->channels && dac3203->unmuted)
		dac3203_apply_mute(codec, 0);

	return 0;
}

static int get_right_ch_enab_only(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	struct dac3203_priv *dac3203 = snd_soc_codec_get_drvdata(codec);

	dev_dbg(codec->dev, "%s channels=%d unmute=%d\n", __func__,
		dac3203->channels, dac3203->unmuted);

	if (dac3203->channels == 1)
		ucontrol->value.enumerated.item[0] = 1;
	else
		ucontrol->value.enumerated.item[0] = 0;

	return 0;
}

static int set_ignore_ramp(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	struct dac3203_priv *dac3203 = snd_soc_codec_get_drvdata(codec);

	dev_info(codec->dev, "%s ignore_set=%d previous_value=%d\n", __func__,
		ucontrol->value.enumerated.item[0], dac3203->ignore_ramp);

	if (ucontrol->value.enumerated.item[0] >
		ARRAY_SIZE(control_enable)) {
		pr_err("%s: Ignore Ramp value Invalid. Value=%d\n", __func__,
			ucontrol->value.enumerated.item[0]);
		return -EINVAL;
	}

	if (ucontrol->value.enumerated.item[0]) {
		/* Headphone Driver Startup Control */
		snd_soc_write(codec, DAC3203_HEADSTART,
			HP_AMP_STARTUP_DELAY_DISABLED);
		dac3203->ignore_ramp = true;
	} else {
		snd_soc_write(codec, DAC3203_HEADSTART,
			HP_AMP_SOFT_ROUTE_STARTUP_DELAY);
		dac3203->ignore_ramp = false;
	}

	return 0;
}

static int get_ignore_ramp(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_kcontrol_chip(kcontrol);
	struct snd_soc_codec *codec = snd_soc_component_to_codec(component);
	struct dac3203_priv *dac3203 = snd_soc_codec_get_drvdata(codec);

	dev_info(codec->dev, "%s ignore_ramp=%d\n", __func__,
		dac3203->ignore_ramp);

	ucontrol->value.enumerated.item[0] = dac3203->ignore_ramp;

	return 0;
}

static const struct snd_kcontrol_new dac3203_snd_controls[] = {
	SOC_DOUBLE_R_S_TLV("PCM Playback Volume", DAC3203_LDACVOL,
			DAC3203_RDACVOL, 0, -0x7f, 0x30, 7, 0, tlv_pcm),
	SOC_DOUBLE_R_S_TLV("HP Driver Gain Volume", DAC3203_HPLGAIN,
			DAC3203_HPRGAIN, 0, -0x6, 0x1d, 5, 0,
			tlv_driver_gain),
	SOC_DOUBLE_R_S_TLV("LO Driver Gain Volume", DAC3203_LOLGAIN,
			DAC3203_LORGAIN, 0, -0x6, 0x1d, 5, 0,
			tlv_driver_gain),
	SOC_DOUBLE_R("HP DAC Playback Switch", DAC3203_HPLGAIN,
			DAC3203_HPRGAIN, 6, 0x01, 1),
	SOC_DOUBLE_R("LO DAC Playback Switch", DAC3203_LOLGAIN,
			DAC3203_LORGAIN, 6, 0x01, 1),
	SOC_DOUBLE_R("Mic PGA Switch", DAC3203_LMICPGAVOL,
			DAC3203_RMICPGAVOL, 7, 0x01, 1),

	SOC_ENUM("DRC Control", dac3203_drc_ctrl_reg1),

	SOC_SINGLE("ADCFGA Left Mute Switch", DAC3203_ADCFGA, 7, 1, 0),
	SOC_SINGLE("ADCFGA Right Mute Switch", DAC3203_ADCFGA, 3, 1, 0),

	SOC_DOUBLE_R_S_TLV("ADC Level Volume", DAC3203_LADCVOL,
			DAC3203_RADCVOL, 0, -0x18, 0x28, 6, 0, tlv_adc_vol),
	SOC_DOUBLE_R_TLV("PGA Level Volume", DAC3203_LMICPGAVOL,
			DAC3203_RMICPGAVOL, 0, 0x5f, 0, tlv_step_0_5),

	SOC_SINGLE("Auto-mute Switch", DAC3203_DACMUTE, 4, 7, 0),

	SOC_SINGLE("AGC Left Switch", DAC3203_LAGC1, 7, 1, 0),
	SOC_SINGLE("AGC Right Switch", DAC3203_RAGC1, 7, 1, 0),
	SOC_DOUBLE_R("AGC Target Level", DAC3203_LAGC1, DAC3203_RAGC1,
			4, 0x07, 0),
	SOC_DOUBLE_R("AGC Gain Hysteresis", DAC3203_LAGC1, DAC3203_RAGC1,
			0, 0x03, 0),
	SOC_DOUBLE_R("AGC Hysteresis", DAC3203_LAGC2, DAC3203_RAGC2,
			6, 0x03, 0),
	SOC_DOUBLE_R("AGC Noise Threshold", DAC3203_LAGC2, DAC3203_RAGC2,
			1, 0x1F, 0),
	SOC_DOUBLE_R("AGC Max PGA", DAC3203_LAGC3, DAC3203_RAGC3,
			0, 0x7F, 0),
	SOC_DOUBLE_R("AGC Attack Time", DAC3203_LAGC4, DAC3203_RAGC4,
			3, 0x1F, 0),
	SOC_DOUBLE_R("AGC Decay Time", DAC3203_LAGC5, DAC3203_RAGC5,
			3, 0x1F, 0),
	SOC_DOUBLE_R("AGC Noise Debounce", DAC3203_LAGC6, DAC3203_RAGC6,
			0, 0x1F, 0),
	SOC_DOUBLE_R("AGC Signal Debounce", DAC3203_LAGC7, DAC3203_RAGC7,
			0, 0x0F, 0),
	SND_SOC_BYTES_EXT("biquad coefficients",
				BIQUAD_SETTINGS_REGS_SIZE,
				dac3203_biquad_coeff_get,
				dac3203_biquad_coeff_set),
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name = "DAC Soft Stepping",
		.info = snd_soc_info_enum_double,
		.get = snd_soc_get_enum_double,
		.put = snd_soc_put_enum_double_wrapper,
		.private_value = (unsigned long)&dac3203_dac_soft_stepping_ctrl
	},
	SOC_ENUM_EXT("Right Channel Only", dac3203_mono_enable_enum,
		get_right_ch_enab_only, put_right_ch_enab_only),
	SOC_ENUM_EXT("Ignore Ramp Up", dac3203_ramp_enable_enum,
		get_ignore_ramp, set_ignore_ramp)
};

static const struct snd_kcontrol_new hpl_output_mixer_controls[] = {
	SOC_DAPM_SINGLE("L_DAC Switch", DAC3203_HPLROUTE, 3, 1, 0),
	SOC_DAPM_SINGLE("IN1_L Switch", DAC3203_HPLROUTE, 2, 1, 0),
};

static const struct snd_kcontrol_new hpr_output_mixer_controls[] = {
	SOC_DAPM_SINGLE("L_DAC Switch", DAC3203_HPRROUTE, 4, 1, 0),
	SOC_DAPM_SINGLE("R_DAC Switch", DAC3203_HPRROUTE, 3, 1, 0),
	SOC_DAPM_SINGLE("IN1_R Switch", DAC3203_HPRROUTE, 2, 1, 0),
};

static const struct snd_kcontrol_new lol_output_mixer_controls[] = {
	SOC_DAPM_SINGLE("L_DAC Switch", DAC3203_LOLROUTE, 3, 1, 0),
};

static const struct snd_kcontrol_new lor_output_mixer_controls[] = {
	SOC_DAPM_SINGLE("R_DAC Switch", DAC3203_LORROUTE, 3, 1, 0),
};

static const struct snd_kcontrol_new left_input_mixer_controls[] = {
	SOC_DAPM_SINGLE("IN1_L P Switch", DAC3203_LMICPGAPIN, 6, 1, 0),
	SOC_DAPM_SINGLE("IN2_L P Switch", DAC3203_LMICPGAPIN, 4, 1, 0),
	SOC_DAPM_SINGLE("IN3_L P Switch", DAC3203_LMICPGAPIN, 2, 1, 0),
};

static const struct snd_kcontrol_new right_input_mixer_controls[] = {
	SOC_DAPM_SINGLE("IN1_R P Switch", DAC3203_RMICPGAPIN, 6, 1, 0),
	SOC_DAPM_SINGLE("IN2_R P Switch", DAC3203_RMICPGAPIN, 4, 1, 0),
	SOC_DAPM_SINGLE("IN3_R P Switch", DAC3203_RMICPGAPIN, 2, 1, 0),
};

static const struct snd_soc_dapm_widget dac3203_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("Left DAC", "Left Playback", DAC3203_DACSETUP, 7, 0),
	SND_SOC_DAPM_MIXER("HPL Output Mixer", SND_SOC_NOPM, 0, 0,
			   &hpl_output_mixer_controls[0],
			   ARRAY_SIZE(hpl_output_mixer_controls)),
	SND_SOC_DAPM_PGA("HPL Power", DAC3203_OUTPWRCTL, 5, 0, NULL, 0),

	SND_SOC_DAPM_MIXER("LOL Output Mixer", SND_SOC_NOPM, 0, 0,
			   &lol_output_mixer_controls[0],
			   ARRAY_SIZE(lol_output_mixer_controls)),
	SND_SOC_DAPM_PGA("LOL Power", DAC3203_OUTPWRCTL, 3, 0, NULL, 0),

	SND_SOC_DAPM_DAC("Right DAC", "Right Playback", DAC3203_DACSETUP, 6, 0),
	SND_SOC_DAPM_MIXER("HPR Output Mixer", SND_SOC_NOPM, 0, 0,
			   &hpr_output_mixer_controls[0],
			   ARRAY_SIZE(hpr_output_mixer_controls)),
	SND_SOC_DAPM_PGA("HPR Power", DAC3203_OUTPWRCTL, 4, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("LOR Output Mixer", SND_SOC_NOPM, 0, 0,
			   &lor_output_mixer_controls[0],
			   ARRAY_SIZE(lor_output_mixer_controls)),
	SND_SOC_DAPM_PGA("LOR Power", DAC3203_OUTPWRCTL, 2, 0, NULL, 0),
	SND_SOC_DAPM_MIXER("Left Input Mixer", SND_SOC_NOPM, 0, 0,
			   &left_input_mixer_controls[0],
			   ARRAY_SIZE(left_input_mixer_controls)),
	SND_SOC_DAPM_MIXER("Right Input Mixer", SND_SOC_NOPM, 0, 0,
			   &right_input_mixer_controls[0],
			   ARRAY_SIZE(right_input_mixer_controls)),
	SND_SOC_DAPM_ADC("Left ADC", "Left Capture", DAC3203_ADCSETUP, 7, 0),
	SND_SOC_DAPM_ADC("Right ADC", "Right Capture", DAC3203_ADCSETUP, 6, 0),
	SND_SOC_DAPM_MICBIAS("Mic Bias", DAC3203_MICBIAS, 6, 0),

	SND_SOC_DAPM_OUTPUT("HPL"),
	SND_SOC_DAPM_OUTPUT("HPR"),
	SND_SOC_DAPM_OUTPUT("LOL"),
	SND_SOC_DAPM_OUTPUT("LOR"),
	SND_SOC_DAPM_INPUT("IN1_L"),
	SND_SOC_DAPM_INPUT("IN1_R"),
	SND_SOC_DAPM_INPUT("IN2_L"),
	SND_SOC_DAPM_INPUT("IN2_R"),
	SND_SOC_DAPM_INPUT("IN3_L"),
	SND_SOC_DAPM_INPUT("IN3_R"),
};

static const struct snd_soc_dapm_route dac3203_dapm_routes[] = {
	/* Left Output */
	{"HPL Output Mixer", "L_DAC Switch", "Left DAC"},
	{"HPL Output Mixer", "IN1_L Switch", "IN1_L"},

	{"HPL Power", NULL, "HPL Output Mixer"},
	{"HPL", NULL, "HPL Power"},

	{"LOL Output Mixer", "L_DAC Switch", "Left DAC"},

	{"LOL Power", NULL, "LOL Output Mixer"},
	{"LOL", NULL, "LOL Power"},

	/* Right Output */
	{"HPR Output Mixer", "R_DAC Switch", "Right DAC"},
	{"HPR Output Mixer", "IN1_R Switch", "IN1_R"},
	{"HPR Output Mixer", "L_DAC Switch", "Left DAC"},

	{"HPR Power", NULL, "HPR Output Mixer"},
	{"HPR", NULL, "HPR Power"},

	{"LOR Output Mixer", "R_DAC Switch", "Right DAC"},

	{"LOR Power", NULL, "LOR Output Mixer"},
	{"LOR", NULL, "LOR Power"},

	/* Left input */
	{"Left Input Mixer", "IN1_L P Switch", "IN1_L"},
	{"Left Input Mixer", "IN2_L P Switch", "IN2_L"},
	{"Left Input Mixer", "IN3_L P Switch", "IN3_L"},

	{"Left ADC", NULL, "Left Input Mixer"},

	/* Right Input */
	{"Right Input Mixer", "IN1_R P Switch", "IN1_R"},
	{"Right Input Mixer", "IN2_R P Switch", "IN2_R"},
	{"Right Input Mixer", "IN3_R P Switch", "IN3_R"},

	{"Right ADC", NULL, "Right Input Mixer"},
};

static const struct regmap_range_cfg dac3203_regmap_pages[] = {
	{
		.selector_reg = 0,
		.selector_mask  = 0xff,
		.window_start = 0,
		.window_len = 128,
		.range_min = 0,
		.range_max = DAC3203_REG_MAX_RANGE,
	},
};

static const struct regmap_config dac3203_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.volatile_reg = dac3203_volatile,
	.max_register = DAC3203_REG_MAX_RANGE,
	.ranges = dac3203_regmap_pages,
	.num_ranges = ARRAY_SIZE(dac3203_regmap_pages),
};

static inline int dac3203_get_divs(int mclk, int rate)
{
	int sample_rate[] = {8000, 11025, 16000, 22050, 32000,
		44100, 48000, 96000, 192000};
	int i;

	for (i = 0; i < ARRAY_SIZE(sample_rate); i++) {
		if ((sample_rate[i] == rate)
		    && (mclk / MCLK_MULT == rate)) {
			return 0;
		}
	}
	pr_err("dac3203:%s master clock %d and sample rate %d is not supported\n",
		__func__, mclk, rate);
	return -EINVAL;
}

static int dac3203_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct dac3203_priv *dac3203 = snd_soc_codec_get_drvdata(codec);

	/* freq = DEFAULT_MCLK; */
	pr_info("dac3203:%s frequency %u\n", __func__, freq);

	switch (freq) {
	case DAC3203_FREQ_8000:
	case DAC3203_FREQ_11025:
	case DAC3203_FREQ_16000:
	case DAC3203_FREQ_22050:
	case DAC3203_FREQ_32000:
	case DAC3203_FREQ_44100:
	case DAC3203_FREQ_48000:
	case DAC3203_FREQ_96000:
	case DAC3203_FREQ_192000:
		dac3203->sysclk = freq;
		return 0;
	}
	pr_err("dac3203:%s invalid frequency %u to set DAI system clock\n",
		__func__, freq);
	return -EINVAL;
}

static int dac3203_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	u8 iface_reg_1;
	u8 iface_reg_2;
	u8 iface_reg_3;

	dev_dbg(codec->dev, "%s: dai fmt\n", __func__);

	iface_reg_1 = snd_soc_read(codec, DAC3203_IFACE1);
	iface_reg_1 = iface_reg_1 & ~(3 << 6 | 3 << 2);
	iface_reg_2 = 0;
	iface_reg_3 = snd_soc_read(codec, DAC3203_IFACE3);
	iface_reg_3 = iface_reg_3 & ~(1 << 3);

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		iface_reg_1 |= DAC3203_BCLKMASTER | DAC3203_WCLKMASTER;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
			iface_reg_3 |= DAC3203_DACMOD2BCLK;
		break;
	default:
		pr_err("dac3203:%s: invalid DAI master/slave interface\n",
			__func__);
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		break;
	case SND_SOC_DAIFMT_DSP_A:
		iface_reg_1 |= (DAC3203_DSP_MODE << DAC3203_PLLJ_SHIFT);
		iface_reg_3 |= (1 << 3); /* invert bit clock */
		iface_reg_2 = 0x01; /* add offset 1 */
		break;
	case SND_SOC_DAIFMT_DSP_B:
		iface_reg_1 |= (DAC3203_DSP_MODE << DAC3203_PLLJ_SHIFT);
		iface_reg_3 |= (1 << 3); /* invert bit clock */
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		iface_reg_1 |=
			(DAC3203_RIGHT_JUSTIFIED_MODE << DAC3203_PLLJ_SHIFT);
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		iface_reg_1 |=
			(DAC3203_LEFT_JUSTIFIED_MODE << DAC3203_PLLJ_SHIFT);
		break;
	default:
		pr_err("dac3203:%s invalid DAI interface format\n", __func__);
		return -EINVAL;
	}

	snd_soc_write(codec, DAC3203_IFACE1, iface_reg_1);
	snd_soc_write(codec, DAC3203_IFACE2, iface_reg_2);
	snd_soc_write(codec, DAC3203_IFACE3, iface_reg_3);
	return 0;
}

static int dac3203_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params,
			     struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct dac3203_priv *dac3203 = snd_soc_codec_get_drvdata(codec);
	u8 data;
	int i, size, width, stream_channels;
	const struct dac3203_configs *pConfigRegs;

	width = params_width(params);
	stream_channels = params_channels(params);
	dev_info(codec->dev, "%s+ stream_ch=%d dac_ch=%d width=%d ignore_ramping=%d\n",
		__func__, stream_channels, dac3203->channels, width,
		dac3203->ignore_ramp);

	i = dac3203_get_divs(dac3203->sysclk, params_rate(params));
	if (i < 0) {
		dev_err(codec->dev, "dac3203: sampling rate not supported\n");
		return i;
	}

	/* Mute HPL Driver to avoid pop  */
	snd_soc_update_bits(codec, DAC3203_HPLGAIN, HP_DRIVER_MUTE_MUX,
		HP_DRIVER_MUTE);
	/* Mute HPR Driver to avoid pop  */
	snd_soc_update_bits(codec, DAC3203_HPRGAIN, HP_DRIVER_MUTE_MUX,
		HP_DRIVER_MUTE);
	snd_soc_write(codec, DAC3203_DACMUTE, 0x04);

	/* turn off PLL */
	dev_info(codec->dev, "dac3203: CLK_SRC uses MCLK\n");
	snd_soc_write(codec, DAC3203_PLLPR, 0);
	need_pll_on = 0;

	/* NDAC divider value */
	snd_soc_write(codec, DAC3203_NDAC, 0x81);

	/* MDAC divider value */
	snd_soc_write(codec, DAC3203_MDAC, 0x82);

	/* DOSR MSB & LSB values */
	snd_soc_write(codec, DAC3203_DOSRMSB, 0);
	snd_soc_write(codec, DAC3203_DOSRLSB, 0x80);

	/* Set and Disable BCLK N divider */
	snd_soc_write(codec, DAC3203_BCLKN, 0x01);

	data = snd_soc_read(codec, DAC3203_IFACE1);
	data = data & ~(3 << 4);
	switch (width) {
	case 16:
		break;
	case 20:
		data |= (DAC3203_WORD_LEN_20BITS << DAC3203_DOSRMSB_SHIFT);
		break;
	case 24:
		data |= (DAC3203_WORD_LEN_24BITS << DAC3203_DOSRMSB_SHIFT);
		break;
	case 32:
		data |= (DAC3203_WORD_LEN_32BITS << DAC3203_DOSRMSB_SHIFT);
		break;
	}
	snd_soc_write(codec, DAC3203_IFACE1, data);

	if (stream_channels == 1) {
		data = DAC3203_RDAC2RCHN | DAC3203_LDAC2RCHN;
	} else {
		if (dac3203->swapdacs)
			data = DAC3203_RDAC2LCHN | DAC3203_LDAC2RCHN;
		else
			data = DAC3203_LDAC2LCHN | DAC3203_RDAC2RCHN;
	}

	/* Set the signal processing block (PRB) modes */
    /* Note for stereo it's 2, for mono it's 4 */
	snd_soc_write(codec, DAC3203_DACSPB, 4);

	/* Program the biquads and DRC */
	pConfigRegs = biquad_Play;
	size = ARRAY_SIZE(biquad_Play);
	for (i = 0; i < size; i++) {
		/* Get the register offset and value */
		snd_soc_write(codec,
			pConfigRegs[i].reg_offset, pConfigRegs[i].reg_val);
	}

	/* Headphone Driver Startup Control if output ramping is enabled */
	snd_soc_write(codec, DAC3203_HEADSTART, 0x01);

	snd_soc_write(codec, DAC3203_DACSETUP, 0x92);

	/* For stereo enable both. Otherwise only enable right channel */
	if (stream_channels == 2) {
		/* Unmute HPL Driver to avoid pop */
		snd_soc_update_bits(codec, DAC3203_HPLGAIN, HP_DRIVER_MUTE_MUX,
			HP_DRIVER_UNMUTE);
	}
	/* Unmute HPR Driver to avoid pop */
	snd_soc_update_bits(codec, DAC3203_HPRGAIN, HP_DRIVER_MUTE_MUX,
		HP_DRIVER_UNMUTE);

	dev_dbg(codec->dev, "%s-\n", __func__);

	return 0;
}

static int dac3203_mute(struct snd_soc_dai *dai, int mute)
{
	return dac3203_apply_mute(dai->codec, mute);
}

static int dac3203_set_bias_level(struct snd_soc_codec *codec,
				  enum snd_soc_bias_level level)
{
	struct dac3203_priv *dac3203 = snd_soc_codec_get_drvdata(codec);
	int ret = 0;

	dev_info(codec->dev, "%s bias=%d\n", __func__, level);

	switch (level) {
	case SND_SOC_BIAS_ON:
		/* Switch on master clock */
		if (dac3203->mclk) {
			ret = clk_prepare_enable(dac3203->mclk);
			if (ret)
				dev_err(codec->dev, "Failed to enable master clock\n");
		}

		/* Switch on PLL */
		if (need_pll_on) {
			snd_soc_update_bits(codec, DAC3203_PLLPR,
					    DAC3203_PLLEN, DAC3203_PLLEN);
		}

		/* Switch on NDAC Divider */
		snd_soc_update_bits(codec, DAC3203_NDAC,
				    DAC3203_NDACEN, DAC3203_NDACEN);

		/* Switch on MDAC Divider */
		snd_soc_update_bits(codec, DAC3203_MDAC,
				    DAC3203_MDACEN, DAC3203_MDACEN);

		/* Switch on NADC Divider */
		snd_soc_update_bits(codec, DAC3203_NADC,
				    DAC3203_NADCEN, DAC3203_NADCEN);

		/* Switch on MADC Divider */
		snd_soc_update_bits(codec, DAC3203_MADC,
				    DAC3203_MADCEN, DAC3203_MADCEN);

		/* Switch on BCLK_N Divider */
		snd_soc_update_bits(codec, DAC3203_BCLKN,
				    DAC3203_BCLKEN, DAC3203_BCLKEN);
		break;
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
		/* Switch off BCLK_N Divider */
		snd_soc_update_bits(codec, DAC3203_BCLKN,
				    DAC3203_BCLKEN, 0);

		/* Switch off MADC Divider */
		snd_soc_update_bits(codec, DAC3203_MADC,
				    DAC3203_MADCEN, 0);

		/* Switch off NADC Divider */
		snd_soc_update_bits(codec, DAC3203_NADC,
				    DAC3203_NADCEN, 0);

		/* Switch off MDAC Divider */
		snd_soc_update_bits(codec, DAC3203_MDAC,
				    DAC3203_MDACEN, 0);

		/* Switch off NDAC Divider */
		snd_soc_update_bits(codec, DAC3203_NDAC,
				    DAC3203_NDACEN, 0);

		/* Switch off PLL */
		if (need_pll_on) {
			snd_soc_update_bits(codec, DAC3203_PLLPR,
					    DAC3203_PLLEN, 0);
		}

		/* Switch off master clock */
		if (dac3203->mclk)
			clk_disable_unprepare(dac3203->mclk);
		break;
	case SND_SOC_BIAS_OFF:
		break;
	}
	codec->component.dapm.bias_level = level;

	return ret;
}

#define DAC3203_RATES SNDRV_PCM_RATE_8000_192000
#define DAC3203_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE \
			 | SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S32_LE)

static const struct snd_soc_dai_ops dac3203_ops = {
	.hw_params = dac3203_hw_params,
	.digital_mute = dac3203_mute,
	.set_sysclk = dac3203_set_dai_sysclk,
	.set_fmt = dac3203_set_dai_fmt,
};

static struct snd_soc_dai_driver dac3203_dai = {
	.name = "tlv320dac3203-hifi",
	.playback = {
		     .stream_name = "Playback",
		     .channels_min = 1,
		     .channels_max = 8,
		     .rates = DAC3203_RATES,
		     .formats = DAC3203_FORMATS,},
	.capture = {
		    .stream_name = "Capture",
		    .channels_min = 1,
		    .channels_max = 2,
		    .rates = DAC3203_RATES,
		    .formats = DAC3203_FORMATS,},
	.ops = &dac3203_ops,
};

static int dac3203_suspend(struct snd_soc_codec *codec)
{
	dac3203_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static int dac3203_resume(struct snd_soc_codec *codec)
{
	dac3203_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	return 0;
}

static int dac3203_init(struct snd_soc_codec *codec)
{
	struct dac3203_priv *dac3203 = snd_soc_codec_get_drvdata(codec);

	pr_info("dac3203: %s\n", __func__);

	/* hw reset: This ensures that i2c is properly read and written. */
	if (gpio_is_valid(dac3203->rstn_gpio)) {
		/* Hold reset down for at least 10nsec */
		gpio_set_value(dac3203->rstn_gpio, 0);
		ndelay(20);
		gpio_set_value(dac3203->rstn_gpio, 1);
		mdelay(10);
		pr_info("dac3203 hw rst\n");
	}

	/* sw reset */
	snd_soc_write(codec, DAC3203_RESET, 0x01);
	/* Required delay after reset */
	mdelay(1);

	/* enable the amp */
	if (gpio_is_valid(dac3203->amp_enable)) {
		gpio_set_value(dac3203->amp_enable, 1);
		pr_info("dac3203 amp enable\n");
	}

	/* set audio path: Mono DAC playback */
	snd_soc_write(codec, DAC3203_HPLROUTE, 0x08);
	snd_soc_write(codec, DAC3203_HPRROUTE, 0x10);

	return 0;
}

static int dac3203_probe(struct snd_soc_codec *codec)
{
	struct dac3203_priv *dac3203 = snd_soc_codec_get_drvdata(codec);
	u32 tmp_reg;

	pr_info("dac3203: %s\n", __func__);

	dac3203_init(codec);

	/* Power platform configuration */
	if (dac3203->power_cfg & AIC32X4_PWR_MICBIAS_2075_LDOIN) {
		snd_soc_write(codec, DAC3203_MICBIAS, DAC3203_MICBIAS_LDOIN |
						      DAC3203_MICBIAS_2075V);
	}
	if (dac3203->power_cfg & AIC32X4_PWR_AVDD_DVDD_WEAK_DISABLE)
		snd_soc_write(codec, DAC3203_PWRCFG, DAC3203_AVDDWEAKDISABLE);

	tmp_reg = (dac3203->power_cfg & AIC32X4_PWR_AIC32X4_LDO_ENABLE) ?
			DAC3203_LDOCTLEN : 0;
	snd_soc_write(codec, DAC3203_LDOCTL, tmp_reg);

	tmp_reg = DAC3203_HP_CMMODE;
	if (dac3203->power_cfg & AIC32X4_PWR_CMMODE_LDOIN_RANGE_18_36)
		tmp_reg |= DAC3203_LDOIN_18_36;
	if (dac3203->power_cfg & AIC32X4_PWR_CMMODE_HP_LDOIN_POWERED)
		tmp_reg |= DAC3203_LDOIN2HP;
	snd_soc_write(codec, DAC3203_CMMODE, tmp_reg);

	/* Mic PGA routing */
	if (dac3203->micpga_routing & AIC32X4_MICPGA_ROUTE_LMIC_IN2R_10K)
		snd_soc_write(codec, DAC3203_LMICPGANIN,
				DAC3203_LMICPGANIN_IN2R_10K);
	else
		snd_soc_write(codec, DAC3203_LMICPGANIN,
				DAC3203_LMICPGANIN_CM1L_10K);
	if (dac3203->micpga_routing & AIC32X4_MICPGA_ROUTE_RMIC_IN1L_10K)
		snd_soc_write(codec, DAC3203_RMICPGANIN,
				DAC3203_RMICPGANIN_IN1L_10K);
	else
		snd_soc_write(codec, DAC3203_RMICPGANIN,
				DAC3203_RMICPGANIN_CM1R_10K);

	dac3203_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

	/*
	 * Workaround: for an unknown reason, the ADC needs to be powered up
	 * and down for the first capture to work properly. It seems related to
	 * a HW BUG or some kind of behavior not documented in the datasheet.
	 */
	tmp_reg = snd_soc_read(codec, DAC3203_ADCSETUP);
	snd_soc_write(codec, DAC3203_ADCSETUP, tmp_reg |
				DAC3203_LADC_EN | DAC3203_RADC_EN);
	snd_soc_write(codec, DAC3203_ADCSETUP, tmp_reg);

	/* Turn off DOUT/MFP2 loopback - Enabled by default */
	snd_soc_write(codec, DAC3203_DOUTCTL, 0);
	/* Disable SCLK/MFP3 */
	snd_soc_write(codec, DAC3203_SCLKMFP, 0);
	/* Set the REF to 40ms */
	snd_soc_write(codec, DAC3203_REF_PWRUP, REF_POWERUP_DELAY);
	/* Set offset calibration mode on first powerup
	 * for mono DAC on differential HP
	 */
	snd_soc_write(codec, DAC3203_OFFSET_CAL, OFFSET_CALIB_FIRST_ONLY);

	return 0;
}

static int dac3203_remove(struct snd_soc_codec *codec)
{
	dac3203_set_bias_level(codec, SND_SOC_BIAS_OFF);
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_dac3203 = {
	.probe = dac3203_probe,
	.remove = dac3203_remove,
	.suspend = dac3203_suspend,
	.resume = dac3203_resume,
	.set_bias_level = dac3203_set_bias_level,

	.component_driver = {
		.controls = dac3203_snd_controls,
		.num_controls = ARRAY_SIZE(dac3203_snd_controls),
		.dapm_widgets = dac3203_dapm_widgets,
		.num_dapm_widgets = ARRAY_SIZE(dac3203_dapm_widgets),
		.dapm_routes = dac3203_dapm_routes,
		.num_dapm_routes = ARRAY_SIZE(dac3203_dapm_routes),
	}
};

static int dac3203_parse_dt(struct dac3203_priv *dac3203,
		struct device_node *np)
{
	pr_info("dac3203: %s\n", __func__);
	dac3203->swapdacs = false;
	dac3203->micpga_routing = 0;
	dac3203->rstn_gpio = of_get_named_gpio(np, "reset-gpios", 0);
	dac3203->amp_enable = of_get_named_gpio(np, "amp-enable", 0);
	dac3203->power_cfg = AIC32X4_PWR_AVDD_DVDD_WEAK_DISABLE |
				AIC32X4_PWR_AIC32X4_LDO_ENABLE |
				AIC32X4_PWR_CMMODE_LDOIN_RANGE_18_36 |
				AIC32X4_PWR_CMMODE_HP_LDOIN_POWERED;

	return 0;
}

#if 0 // remove them since they are not used now
static void dac3203_disable_regulators(struct dac3203_priv *dac3203)
{
	regulator_disable(dac3203->supply_iov);

	if (!IS_ERR(dac3203->supply_ldo))
		regulator_disable(dac3203->supply_ldo);

	if (!IS_ERR(dac3203->supply_dv))
		regulator_disable(dac3203->supply_dv);

	if (!IS_ERR(dac3203->supply_av))
		regulator_disable(dac3203->supply_av);
}

static int dac3203_setup_regulators(struct device *dev,
		struct dac3203_priv *dac3203)
{
	int ret = 0;

	/* Those was using devm_regulator_get_optional()
	 * besides "iov", need to double check
	 */
	dac3203->supply_ldo = regulator_get(dev, "ldoin");
	dac3203->supply_iov = regulator_get(dev, "iov");
	dac3203->supply_dv = regulator_get(dev, "dv");
	dac3203->supply_av = regulator_get(dev, "av");

	/* Check if the regulator requirements are fulfilled */

	if (IS_ERR(dac3203->supply_iov)) {
		dev_err(dev, "Missing supply 'iov'\n");
		return PTR_ERR(dac3203->supply_iov);
	}

	if (IS_ERR(dac3203->supply_ldo)) {
		if (PTR_ERR(dac3203->supply_ldo) == -EPROBE_DEFER)
			return -EPROBE_DEFER;

		if (IS_ERR(dac3203->supply_dv)) {
			dev_err(dev, "Missing supply 'dv' or 'ldoin'\n");
			return PTR_ERR(dac3203->supply_dv);
		}
		if (IS_ERR(dac3203->supply_av)) {
			dev_err(dev, "Missing supply 'av' or 'ldoin'\n");
			return PTR_ERR(dac3203->supply_av);
		}
	} else {
		if (IS_ERR(dac3203->supply_dv) &&
				PTR_ERR(dac3203->supply_dv) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
		if (IS_ERR(dac3203->supply_av) &&
				PTR_ERR(dac3203->supply_av) == -EPROBE_DEFER)
			return -EPROBE_DEFER;
	}

	ret = regulator_enable(dac3203->supply_iov);
	if (ret) {
		dev_err(dev, "Failed to enable regulator iov\n");
		return ret;
	}

	if (!IS_ERR(dac3203->supply_ldo)) {
		ret = regulator_enable(dac3203->supply_ldo);
		if (ret) {
			dev_err(dev, "Failed to enable regulator ldo\n");
			goto error_ldo;
		}
	}

	if (!IS_ERR(dac3203->supply_dv)) {
		ret = regulator_enable(dac3203->supply_dv);
		if (ret) {
			dev_err(dev, "Failed to enable regulator dv\n");
			goto error_dv;
		}
	}

	if (!IS_ERR(dac3203->supply_av)) {
		ret = regulator_enable(dac3203->supply_av);
		if (ret) {
			dev_err(dev, "Failed to enable regulator av\n");
			goto error_av;
		}
	}

	if (!IS_ERR(dac3203->supply_ldo) && IS_ERR(dac3203->supply_av))
		dac3203->power_cfg |= DAC3203_PWR_DAC3203_LDO_ENABLE;

	return 0;

error_av:
	if (!IS_ERR(dac3203->supply_dv))
		regulator_disable(dac3203->supply_dv);

error_dv:
	if (!IS_ERR(dac3203->supply_ldo))
		regulator_disable(dac3203->supply_ldo);

error_ldo:
	regulator_disable(dac3203->supply_iov);
	return ret;
}
#endif

static int dac3203_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct dac3203_priv *dac3203 = NULL;
	struct device_node *np = i2c->dev.of_node;
	int ret;

	pr_info("dac3203: %s\n", __func__);

	dac3203 = devm_kzalloc(&i2c->dev, sizeof(struct dac3203_priv),
			       GFP_KERNEL);
	if (dac3203 == NULL)
		return -ENOMEM;

	/* Default to stereo */
	dac3203->channels = 2;
	/* Default to not ignore ramp up time of DAC */
	dac3203->ignore_ramp = false;

	dac3203->regmap = devm_regmap_init_i2c(i2c, &dac3203_regmap);
	if (IS_ERR(dac3203->regmap)) {
		dev_err(&i2c->dev, "Failed to allocate register map\n");
		return PTR_ERR(dac3203->regmap);
	}

	i2c_set_clientdata(i2c, dac3203);
	dac3203->dev = i2c;

	mutex_init(&i2c_access);

	if (np) {
		ret = dac3203_parse_dt(dac3203, np);
		if (ret) {
			dev_err(&i2c->dev, "Failed to parse DT node\n");
			return ret;
		}
	} else {
		dac3203->power_cfg = 0;
		dac3203->swapdacs = false;
		dac3203->micpga_routing = 0;
		dac3203->rstn_gpio = -1;
		dac3203->amp_enable = -1;
	}

	/* clock already enabled in machine driver. no need to do below */
	dac3203->mclk = 0;

	if (gpio_is_valid(dac3203->rstn_gpio)) {
		ret = devm_gpio_request_one(&i2c->dev, dac3203->rstn_gpio,
				GPIOF_OUT_INIT_LOW, "tlv320dac3203 rstn");
		if (ret != 0)
			return ret;
	}

	if (gpio_is_valid(dac3203->amp_enable)) {
		ret = devm_gpio_request_one(&i2c->dev, dac3203->amp_enable,
				GPIOF_OUT_INIT_LOW, "tlv320dac3203 amp enable");
		if (ret != 0)
			return ret;
	}

#if 0
	/* TODO: Enable regulators if needed */
	ret = dac3203_setup_regulators(&i2c->dev, dac3203);
	if (ret) {
		dev_err(&i2c->dev, "Failed to setup regulators\n");
		return ret;
	}
#endif

	ret = snd_soc_register_codec(&i2c->dev,
			&soc_codec_dev_dac3203, &dac3203_dai, 1);
	if (ret) {
		dev_err(&i2c->dev, "Failed to register codec\n");
		/* TODO: Disable regulators if needed
		 * DAC3203_disable_regulators(dac3203);
		 */
		return ret;
	}

	i2c_set_clientdata(i2c, dac3203);

	pr_info("dac3203: %s end\n", __func__);

	return 0;
}

static int dac3203_i2c_remove(struct i2c_client *client)
{
	/* TODO: Disable regulators if needed
	 * struct dac3203_priv *dac3203 = i2c_get_clientdata(client);
	 * dac3203_disable_regulators(dac3203);
	 */

	snd_soc_unregister_codec(&client->dev);
	return 0;
}

static const struct i2c_device_id dac3203_i2c_id[] = {
	{ "tlv320dac3203", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, dac3203_i2c_id);

static const struct of_device_id dac3203_of_id[] = {
	{ .compatible = "ti, tlv320dac3203", },
	{ /* senitel */ }
};
MODULE_DEVICE_TABLE(of, dac3203_of_id);

static struct i2c_driver dac3203_i2c_driver = {
	.driver = {
		.name = "tlv320dac3203",
		.owner = THIS_MODULE,
		.of_match_table = dac3203_of_id,
	},
	.probe =    dac3203_i2c_probe,
	.remove =   dac3203_i2c_remove,
	.id_table = dac3203_i2c_id,
};

module_i2c_driver(dac3203_i2c_driver);

MODULE_DESCRIPTION("ASoC tlv320dac3203 codec driver");
MODULE_AUTHOR("Xing Fang <xing.fang@amlogic.com>");
MODULE_LICENSE("GPL");

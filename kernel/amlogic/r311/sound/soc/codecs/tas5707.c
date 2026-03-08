#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/tas57xx.h>
#include <linux/amlogic/aml_gpio_consumer.h>

#include "tas5707.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static void tas5707_early_suspend(struct early_suspend *h);
static void tas5707_late_resume(struct early_suspend *h);
#endif

#define tas5707_RATES (SNDRV_PCM_RATE_8000 | \
		       SNDRV_PCM_RATE_11025 | \
		       SNDRV_PCM_RATE_16000 | \
		       SNDRV_PCM_RATE_22050 | \
		       SNDRV_PCM_RATE_32000 | \
		       SNDRV_PCM_RATE_44100 | \
		       SNDRV_PCM_RATE_48000)

#define tas5707_FORMATS \
	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S16_BE | \
	 SNDRV_PCM_FMTBIT_S20_3LE | SNDRV_PCM_FMTBIT_S20_3BE | \
	 SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S24_BE | \
	 SNDRV_PCM_FMTBIT_S32_LE)

/* Power-up register defaults */
static const u8 tas5707_regs[DDX_NUM_BYTE_REG] = {
	0x6c, 0x70, 0x00, 0xA0, 0x05, 0x40, 0x00, 0xFF,
	0x30, 0x30, 0xFF, 0x00, 0x00, 0x00, 0x91, 0x00,
	0x02, 0xAC, 0x54, 0xAC, 0x54, 0x00, 0x00, 0x00,
	0x00, 0x30, 0x0F, 0x82, 0x02,
};

static unsigned tas5707_EQ_table_length = 280;
static unsigned tas5707_EQ_table[280] = {
	/*0x29---ch1_bq[0]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x2A---ch1_bq[1]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x2B---ch1_bq[2]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x2C---ch1_bq[3]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x2D---ch1_bq[4]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x2E---ch1_bq[5]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x2F---ch1_bq[6]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x30---ch2_bq[0]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x31---ch2_bq[1]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x32---ch2_bq[2]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x33---ch2_bq[3]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x34---ch2_bq[4]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x35---ch2_bq[5]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/*0x36---ch2_bq[6]*/
	0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static unsigned tas5707_drc1_table_length = 24;
static unsigned tas5707_drc1_table[24] = {
	/* 0x3A drc1_ae */
	0x00, 0x02, 0xA3, 0x9A, 0x00, 0x7D, 0x5C, 0x65,
	/* 0x3B drc1_aa */
	0x00, 0x7F, 0xFF, 0x04, 0x00, 0x00, 0x00, 0xFB,
	/* 0x3C drc1_ad */
	0x00, 0x00, 0x01, 0x5D, 0x00, 0x7F, 0xFE, 0xA2,
};

static unsigned tas5707_drc1_tko_length = 12;
static unsigned tas5707_drc1_tko_table[12] = {
	0xFD, 0x97, 0x73, 0x3d, /*0x40---drc1_t*/
	0x0F, 0x7E, 0xB8, 0x52, /*0x41---drc1_k*/
	0x00, 0x08, 0x42, 0x10, /*0x42---drc1_o*/
};

unsigned char burst_data_1[3][4] = {
	{ 0x00, 0x01, 0x77, 0x72 },
	{ 0x00, 0x00, 0x42, 0x03 },
	{ 0x01, 0x02, 0x13, 0x45 },
};

unsigned char burst_data_2[3][4] = {
	{ 0x00, 0x89, 0x77, 0x72 },
	{ 0x00, 0x00, 0x42, 0x03 },
	{ 0x01, 0x20, 0x13, 0x45 },
};

#define UNKNOW_BOARD_ID_TYPE 0
#define HVT1_BOARD_ID_TYPE 1
#define HVT2_BOARD_ID_TYPE 2
#define EVT_BOARD_ID_TYPE 3
#define DVT_BOARD_ID_TYPE 4
#define PVT_BOARD_ID_TYPE 5

#define HVT1_BOARD_ID_TYPO "40001100000017"
#define HVT1_BOARD_ID "0040001100000017"

int get_board_rev(void)
{
	int type;
	struct device_node *ap = NULL;
	char board_type[64] = { 0 };
#ifdef CONFIG_ARM64
	const char *board_id = NULL;
#else
	const char *board_id = "NULL"; /*TODO TEMP,  Crash without board_id*/
#endif

	ap = of_find_node_by_path("/idme/board_id");
	if (ap)
		board_id = (const char *)of_get_property(ap, "value", NULL);
	else
		pr_err("of_find_node_by_path failed\n");

	strlcpy(board_type, board_id, sizeof(board_type));

	if ((0 == strcmp(board_type, HVT1_BOARD_ID_TYPO))
		|| (0 == strcmp(board_type, HVT1_BOARD_ID)))
		type = HVT1_BOARD_ID_TYPE;
	else
		type = HVT2_BOARD_ID_TYPE;

	pr_info("get_board_rev board_type = %s;type = %x\n", board_type, type);
	return type;
}

enum tas5707_state {
	TAS5707_INIT = 0,
	TAS5707_SUSPEND,
	TAS5707_RESUME
};

/* codec private data */
struct tas5707_priv {
	struct delayed_work tas5707_delayed_work;

	struct snd_soc_codec *codec;
	struct tas57xx_platform_data *pdata;

	enum snd_soc_control_type control_type;
	void *control_data;

	/*Platform provided EQ configuration */
	int num_eq_conf_texts;
	const char **eq_conf_texts;
	int eq_cfg;
	struct soc_enum eq_conf_enum;
	unsigned char Ch1_vol;
	unsigned char Ch2_vol;
	unsigned char master_vol;
	unsigned int reg_mute;
	unsigned mclk;
	struct early_suspend early_suspend;

	enum tas5707_state state;
};

static int tas5707_set_EQ_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol);
static int tas5707_get_EQ_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol);
static int tas5707_set_DRC_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol);
static int tas5707_get_DRC_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol);
static int tas5707_get_model_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol);
static int tas5707_set_model_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol);

static const DECLARE_TLV_DB_SCALE(mvol_tlv, -12700, 50, 1);
static const DECLARE_TLV_DB_SCALE(chvol_tlv, -10300, 50, 1);

static const char *const model_id_texts[] = {"AD", "BD"};
static const struct soc_enum model_id_enum =
	SOC_ENUM_SINGLE(SND_SOC_NOPM, 0,
			   ARRAY_SIZE(model_id_texts), model_id_texts);

static const struct snd_kcontrol_new tas5707_snd_controls[] = {
	SOC_SINGLE_TLV("Master Volume", DDX_MASTER_VOLUME, 0,
			   0xff, 1, mvol_tlv),
	SOC_SINGLE_TLV("Ch1 Volume", DDX_CHANNEL1_VOL, 0,
			   0xff, 1, chvol_tlv),
	SOC_SINGLE_TLV("Ch2 Volume", DDX_CHANNEL2_VOL, 0,
			   0xff, 1, chvol_tlv),
	SOC_SINGLE("Ch1 Switch", DDX_SOFT_MUTE, 0, 1, 1),
	SOC_SINGLE("Ch2 Switch", DDX_SOFT_MUTE, 1, 1, 1),
	SOC_SINGLE_RANGE("Fine Master Volume", DDX_CHANNEL3_VOL, 0,
			   0x80, 0x83, 0),
	SOC_SINGLE_BOOL_EXT("Set EQ Enable", 0,
			   tas5707_get_EQ_enum, tas5707_set_EQ_enum),
	SOC_SINGLE_BOOL_EXT("Set DRC Enable", 0,
			   tas5707_get_DRC_enum, tas5707_set_DRC_enum),
	SOC_ENUM_EXT("Set Model ID", model_id_enum,
			   tas5707_get_model_enum, tas5707_set_model_enum),
};

static int tas5707_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct tas5707_priv *tas5707 = snd_soc_codec_get_drvdata(codec);

	tas5707->mclk = freq;
	/* 0x74 = 512fs; 0x6c = 256fs */
	if (freq == 512 * 48000)
		snd_soc_write(codec, DDX_CLOCK_CTL, 0x74);
	else
		snd_soc_write(codec, DDX_CLOCK_CTL, 0x6c);
	return 0;
}

static int tas5707_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_NB_IF:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int tas5707_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params,
			     struct snd_soc_dai *dai)
{
	unsigned int rate;

	rate = params_rate(params);
	pr_debug("rate: %u\n", rate);

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S24_BE:
		pr_debug("24bit\n");
	/* fall through */
	case SNDRV_PCM_FORMAT_S32_LE:
	case SNDRV_PCM_FORMAT_S20_3LE:
	case SNDRV_PCM_FORMAT_S20_3BE:
		pr_debug("20bit\n");

		break;
	case SNDRV_PCM_FORMAT_S16_LE:
	case SNDRV_PCM_FORMAT_S16_BE:
		pr_debug("16bit\n");

		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int tas5707_set_bias_level(struct snd_soc_codec *codec,
				  enum snd_soc_bias_level level)
{
	pr_debug("level = %d\n", level);
	switch (level) {
	case SND_SOC_BIAS_ON:
		break;

	case SND_SOC_BIAS_PREPARE:
		/* Full power on */
		break;

	case SND_SOC_BIAS_STANDBY:
		break;

	case SND_SOC_BIAS_OFF:
		/* The chip runs through the power down sequence for us. */
		break;
	}
	codec->dapm.bias_level = level;
	return 0;
}

static const struct snd_soc_dai_ops tas5707_dai_ops = {
	.hw_params = tas5707_hw_params,
	.set_sysclk = tas5707_set_dai_sysclk,
	.set_fmt = tas5707_set_dai_fmt,
};

static struct snd_soc_dai_driver tas5707_dai = {
	.name = "tas5707",
	.playback = {
		.stream_name = "HIFI Playback",
		.channels_min = 2,
		.channels_max = 8,
		.rates = tas5707_RATES,
		.formats = tas5707_FORMATS,
	},
	.ops = &tas5707_dai_ops,
};

static int tas5707_set_master_vol(struct snd_soc_codec *codec)
{
	struct tas57xx_platform_data *pdata = dev_get_platdata(codec->dev);

	/* using user BSP defined master vol config; */
	if (pdata && pdata->custom_master_vol) {
		pr_debug("tas5707_set_master_vol::%d\n",
			pdata->custom_master_vol);
		snd_soc_write(codec, DDX_MASTER_VOLUME,
			      (0xff - pdata->custom_master_vol));
	} else {
		snd_soc_write(codec, DDX_MASTER_VOLUME, 0x69);
	}

	return 0;
}

/* tas5707 DRC for channel L/R */
static int tas5707_set_drc1(struct snd_soc_codec *codec)
{
	int i = 0, j = 0;
	unsigned *p = NULL;
	u8 tas5707_drc1_table_tmp[8];
	u8 tas5707_drc1_tko_table_tmp[4];

	p = &tas5707_drc1_table[0];
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 8; j++)
			tas5707_drc1_table_tmp[j] = p[i * 8 + j];

		regmap_raw_write(codec->control_data, DDX_DRC1_AE + i,
					tas5707_drc1_table_tmp, 8);
		/*for (j = 0; j < 8; j++)
			pr_info("TAS5707_drc1_table[%d][%d]: %x\n",
					i, j, tas5707_drc1_table_tmp[j]);*/
	}

	p = &tas5707_drc1_tko_table[0];
	for (i = 0; i < 3; i++) {
		for (j = 0; j < 4; j++)
			tas5707_drc1_tko_table_tmp[j] = p[i * 4 + j];

		regmap_raw_write(codec->control_data, DDX_DRC1_T + i,
					tas5707_drc1_tko_table_tmp, 4);
		/*for (j = 0; j < 4; j++)
			pr_info("tas5707_drc1_tko_table[%d][%d]: %x\n",
					i, j, tas5707_drc1_tko_table_tmp[j]);*/
	}
	return 0;
}

static int tas5707_set_drc(struct snd_soc_codec *codec)
{
	char drc_mask = 0;
	u8 tas5707_drc_ctl_table[] = { 0x00, 0x00, 0x00, 0x00 };

	regmap_raw_write(codec->control_data, DDX_DRC_CTL,
		tas5707_drc_ctl_table, 4);
	drc_mask |= 0x01;
	tas5707_drc_ctl_table[3] = drc_mask;
	tas5707_set_drc1(codec);
	regmap_raw_write(codec->control_data, DDX_DRC_CTL,
		tas5707_drc_ctl_table, 4);
	return 0;
}

static int tas5707_set_eq_biquad(struct snd_soc_codec *codec)
{
	int i = 0, j = 0, k = 0;
	unsigned *p = NULL;
	u8 addr;
	u8 tas5707_bq_table[20];

	p = &tas5707_EQ_table[0];

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 7; j++) {
			addr = (DDX_CH1_BQ_0 + i * 7 + j);
			for (k = 0; k < 20; k++)
				tas5707_bq_table[k] =
					p[i * 7 * 20 + j * 20 + k];
			regmap_raw_write(codec->control_data, addr,
					tas5707_bq_table, 20);
			/*for (k = 0; k < 20; k++)
				pr_info("tas5707_bq_table[%d]: %x\n",
					k, tas5707_bq_table[k]);*/
		}
	}
	return 0;
}

static int tas5707_set_eq(struct snd_soc_codec *codec)
{
	u8 tas5707_eq_ctl_table[] = { 0x00, 0x00, 0x00, 0x80 };

	regmap_raw_write(codec->control_data, DDX_BANKSWITCH_AND_EQCTL,
			 tas5707_eq_ctl_table, 4);
	tas5707_set_eq_biquad(codec);
	tas5707_eq_ctl_table[3] &= 0x7F;
	regmap_raw_write(codec->control_data, DDX_BANKSWITCH_AND_EQCTL,
			 tas5707_eq_ctl_table, 4);
	return 0;
}

bool EQ_enum_value = 0;
static int tas5707_set_EQ_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	u8 tas5707_eq_ctl_table[] = { 0x00, 0x00, 0x00, 0x80 };
	EQ_enum_value = ucontrol->value.integer.value[0];

	if (EQ_enum_value == 1)
		tas5707_set_eq(codec);
	else
		regmap_raw_write(codec->control_data,
			DDX_BANKSWITCH_AND_EQCTL,
			tas5707_eq_ctl_table, 4);

	return 0;
}
static int tas5707_get_EQ_enum(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = EQ_enum_value;
	return 0;
}

bool DRC_enum_value = 1;
static int tas5707_set_DRC_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	u8 tas5707_drc_ctl_table[] = { 0x00, 0x00, 0x00, 0x00 };
	DRC_enum_value = ucontrol->value.integer.value[0];

	if (DRC_enum_value == 1)
		tas5707_set_drc(codec);
	else
		regmap_raw_write(codec->control_data, DDX_DRC_CTL,
			tas5707_drc_ctl_table, 4);

	return 0;
}
static int tas5707_get_DRC_enum(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = DRC_enum_value;
	return 0;
}
static int model_enum_value;
static int tas5707_get_model_enum(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	int value = model_enum_value;
#if 0
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	u8 model_table[4] = {0};
	regmap_raw_read(codec->control_data, DDX_IC_DELAY_CHANNEL_1,
				model_table, 4);
	dev_info(codec->dev, "%s: 0x%X 0x%X 0x%X 0x%X\n", __func__,
		model_table[0], model_table[1], model_table[2], model_table[3]);
#endif
	ucontrol->value.enumerated.item[0] = value;
	return 0;
}
static int tas5707_set_model_enum(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	u8 AD_model_table[] = { 0xAC, 0x54, 0xAC, 0x54 };
	u8 BD_model_table[] = { 0xB8, 0x60, 0xA0, 0x48 };

	if (ucontrol->value.enumerated.item[0] == 0) {
		regmap_raw_write(codec->control_data, DDX_IC_DELAY_CHANNEL_1,
			AD_model_table, 4);
	} else if (ucontrol->value.enumerated.item[0] == 1) {
		regmap_raw_write(codec->control_data, DDX_IC_DELAY_CHANNEL_1,
			BD_model_table, 4);
	}

	model_enum_value = ucontrol->value.enumerated.item[0];
	return 0;
}
static int reset_tas5707_GPIO(struct tas5707_priv *tas5707)
{
	struct tas57xx_platform_data *pdata = tas5707->pdata;
	int ret = 0;

	gpio_direction_output(pdata->reset_pin, GPIOF_OUT_INIT_LOW);
	gpio_direction_output(pdata->pdn_pin, GPIOF_OUT_INIT_HIGH);
	udelay(1000);
	gpio_direction_output(pdata->reset_pin, GPIOF_OUT_INIT_HIGH);

	if (pdata->clk_mclk) {
		pr_info("reset_tas5707_GPIO mclk enable\n");
		ret = clk_prepare_enable(pdata->clk_mclk);
		if (ret)
			pr_info("Can't enable I2S mclk clock: %d\n", ret);
	}

	schedule_delayed_work(&tas5707->tas5707_delayed_work,
				msecs_to_jiffies(15));

	return 0;
}

static int tas5707_init(struct tas5707_priv *tas5707)
{
	int i = 0, j = 0;
	struct snd_soc_codec *codec = tas5707->codec;
	int retval = get_board_rev();
	unsigned char burst_data[3][4] = {{0}, {0}, {0},};
	if (retval == HVT1_BOARD_ID_TYPE) {
		for (i = 0; i < 3; i++) {
			for (j = 0; j < 4; j++)
				burst_data[i][j] = burst_data_1[i][j];
		}

	} else {
		for (i = 0; i < 3; i++) {
			for (j = 0; j < 4; j++)
				burst_data[i][j] = burst_data_2[i][j];
		}
	}

	dev_info(codec->dev, "tas5707_init!\n");
	snd_soc_write(codec, DDX_OSC_TRIM, 0x00);
	msleep(50);
	snd_soc_write(codec, DDX_CLOCK_CTL, 0x6c);
	snd_soc_write(codec, DDX_SYS_CTL_1, 0xa0);
	snd_soc_write(codec, DDX_SERIAL_DATA_INTERFACE, 0x05);
	snd_soc_write(codec, DDX_BKND_ERR, 0x02);

	regmap_raw_write(codec->control_data, DDX_INPUT_MUX, burst_data[0], 4);
	regmap_raw_write(codec->control_data, DDX_CH4_SOURCE_SELECT,
			 burst_data[1], 4);
	regmap_raw_write(codec->control_data, DDX_PWM_MUX, burst_data[2], 4);

	/*drc */
	/*tas5707_set_drc(codec);*/
	/*eq */
	/*tas5707_set_eq(codec);*/

	snd_soc_write(codec, DDX_VOLUME_CONFIG, 0xD1);
	snd_soc_write(codec, DDX_SYS_CTL_2, 0x84);
	snd_soc_write(codec, DDX_START_STOP_PERIOD, 0x95);
	snd_soc_write(codec, DDX_PWM_SHUTDOWN_GROUP, 0x30);
	snd_soc_write(codec, DDX_MODULATION_LIMIT, 0x02);

	/*normal operation */
	if ((tas5707_set_master_vol(codec)) < 0)
		dev_err(codec->dev, "fail to set tas5707 master vol!\n");

	snd_soc_write(codec, DDX_CHANNEL1_VOL, tas5707->Ch1_vol);
	snd_soc_write(codec, DDX_CHANNEL2_VOL, tas5707->Ch2_vol);
	snd_soc_write(codec, DDX_SOFT_MUTE, 0x00);
	snd_soc_write(codec, DDX_CHANNEL3_VOL, 0x80);

	return 0;
}

static void tas5707_delayed_work_func(struct work_struct *work)
{
	struct tas5707_priv *tas5707 = container_of(work,
				struct tas5707_priv, tas5707_delayed_work.work);
	struct snd_soc_codec *codec = tas5707->codec;
	u8 tas5707_eq_ctl_table[] = { 0x00, 0x00, 0x00, 0x80 };
	u8 tas5707_drc_ctl_table[] = { 0x00, 0x00, 0x00, 0x00 };

	tas5707_init(tas5707);
	if (tas5707->state == TAS5707_RESUME) {
		/* restore the STR register vals */
		snd_soc_write(codec, DDX_CHANNEL1_VOL, tas5707->Ch1_vol);
		snd_soc_write(codec, DDX_CHANNEL2_VOL, tas5707->Ch2_vol);
		snd_soc_write(codec, DDX_MASTER_VOLUME, tas5707->master_vol);
		snd_soc_write(codec, DDX_SOFT_MUTE, tas5707->reg_mute);
		tas5707_set_bias_level(codec, SND_SOC_BIAS_STANDBY);

		pr_info("tas5707 resume from STR\n");
		pr_info("tas5707 start to restore eq & drc\n");
		if (EQ_enum_value == 1)
			tas5707_set_eq(codec);
		else
			regmap_raw_write(codec->control_data,
					DDX_BANKSWITCH_AND_EQCTL,
					tas5707_eq_ctl_table, 4);

		if (DRC_enum_value == 1)
			tas5707_set_drc(codec);
		else
			regmap_raw_write(codec->control_data, DDX_DRC_CTL,
					tas5707_drc_ctl_table, 4);
		pr_info("tas5707 finished restore eq & drc\n");
	}
}

static int tas5707_probe(struct snd_soc_codec *codec)
{
	int ret = 0;
	struct tas5707_priv *tas5707 = snd_soc_codec_get_drvdata(codec);
	struct tas57xx_platform_data *pdata = dev_get_platdata(codec->dev);

#ifdef CONFIG_HAS_EARLYSUSPEND
	tas5707->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	tas5707->early_suspend.suspend = tas5707_early_suspend;
	tas5707->early_suspend.resume = tas5707_late_resume;
	tas5707->early_suspend.param = codec;
	register_early_suspend(&(tas5707->early_suspend));
#endif

	tas5707->pdata = pdata;
	tas5707->codec = codec;
	tas5707->state = TAS5707_INIT;
	ret = snd_soc_codec_set_cache_io(codec, 8, 8, tas5707->control_type);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to set cache I/O: %d\n", ret);
		return ret;
	}

	INIT_DELAYED_WORK(&tas5707->tas5707_delayed_work,
				tas5707_delayed_work_func);

	reset_tas5707_GPIO(tas5707);
	return 0;
}

static int tas5707_remove(struct snd_soc_codec *codec)
{
	struct tas5707_priv *tas5707 = snd_soc_codec_get_drvdata(codec);
	struct tas57xx_platform_data *pdata = dev_get_platdata(codec->dev);

	gpio_direction_output(pdata->reset_pin, GPIOF_OUT_INIT_LOW);
	udelay(10);
	gpio_direction_output(pdata->pdn_pin, GPIOF_OUT_INIT_LOW);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&(tas5707->early_suspend));
#endif

	return 0;
}

#ifdef CONFIG_PM
static int tas5707_suspend(struct snd_soc_codec *codec)
{
	struct tas5707_priv *tas5707 = snd_soc_codec_get_drvdata(codec);
	struct tas57xx_platform_data *pdata = dev_get_platdata(codec->dev);

	dev_info(codec->dev, "tas5707_suspend!\n");

	tas5707->state = TAS5707_SUSPEND;

	if (pdata && pdata->suspend_func)
		pdata->suspend_func();

	/* save volume and mute */
	tas5707->Ch1_vol = snd_soc_read(codec, DDX_CHANNEL1_VOL);
	tas5707->Ch2_vol = snd_soc_read(codec, DDX_CHANNEL2_VOL);
	tas5707->master_vol = snd_soc_read(codec, DDX_MASTER_VOLUME);
	tas5707->reg_mute = snd_soc_read(codec, DDX_SOFT_MUTE);
	tas5707_set_bias_level(codec, SND_SOC_BIAS_OFF);

	gpio_direction_output(pdata->reset_pin, GPIOF_OUT_INIT_LOW);
	udelay(10);
	gpio_direction_output(pdata->pdn_pin, GPIOF_OUT_INIT_LOW);
	udelay(5);
	if (pdata->clk_mclk) {
		pr_info("tas5707_suspend mclock\n");
		clk_disable_unprepare(pdata->clk_mclk);
	}
	return 0;
}

static int tas5707_resume(struct snd_soc_codec *codec)
{
	struct tas5707_priv *tas5707 = snd_soc_codec_get_drvdata(codec);
	struct tas57xx_platform_data *pdata = dev_get_platdata(codec->dev);

	dev_info(codec->dev, "tas5707_resume!\n");

	tas5707->state = TAS5707_RESUME;

	if (pdata && pdata->resume_func)
		pdata->resume_func();

	reset_tas5707_GPIO(tas5707);
	return 0;
}
#else
#define tas5707_suspend NULL
#define tas5707_resume NULL
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void tas5707_early_suspend(struct early_suspend *h)
{
	struct snd_soc_codec *codec = (struct snd_soc_codec *)h->param;

	dev_info(codec->dev, "tas5707_early_suspend!\n");

	return;
}

static void tas5707_late_resume(struct early_suspend *h)
{
	struct snd_soc_codec *codec = (struct snd_soc_codec *)h->param;

	dev_info(codec->dev, "tas5707_late_resume!\n");

	return;
}
#endif

static const struct snd_soc_dapm_widget tas5707_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("DAC", "HIFI Playback", SND_SOC_NOPM, 0, 0),
};

static const struct snd_soc_codec_driver tas5707_codec = {
	.probe = tas5707_probe,
	.remove = tas5707_remove,
	.suspend = tas5707_suspend,
	.resume = tas5707_resume,
	.reg_cache_size = DDX_NUM_BYTE_REG,
	.reg_word_size = sizeof(u8),
	.reg_cache_default = tas5707_regs,
	.set_bias_level = tas5707_set_bias_level,
	.controls = tas5707_snd_controls,
	.num_controls = ARRAY_SIZE(tas5707_snd_controls),
	.dapm_widgets = tas5707_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(tas5707_dapm_widgets),
};

static int tas5707_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct tas5707_priv *tas5707;
	int ret;

	tas5707 = devm_kzalloc(&i2c->dev, sizeof(struct tas5707_priv),
			       GFP_KERNEL);
	if (!tas5707)
		return -ENOMEM;

	i2c_set_clientdata(i2c, tas5707);
	tas5707->control_type = SND_SOC_I2C;

	ret = snd_soc_register_codec(&i2c->dev, &tas5707_codec,
				     &tas5707_dai, 1);
	if (ret != 0)
		dev_err(&i2c->dev, "Failed to register codec (%d)\n", ret);

	return ret;
}

static int tas5707_i2c_remove(struct i2c_client *client)
{
	devm_kfree(&client->dev, i2c_get_clientdata(client));
	return 0;
}

static const struct i2c_device_id tas5707_i2c_id[] = {
	{ "tas5707", 0 },
	{}
};

static struct i2c_driver tas5707_i2c_driver = {
	.driver = {
		.name = "tas5707",
		.owner = THIS_MODULE,
	},
	.probe = tas5707_i2c_probe,
	.remove = tas5707_i2c_remove,
	.id_table = tas5707_i2c_id,
};

static int aml_tas5707_codec_probe(struct platform_device *pdev)
{
	return 0;
}
static int aml_tas5707_codec_remove(struct platform_device *pdev)
{
	return 0;
}
static const struct of_device_id amlogic_tas5707_codec_dt_match[] = {
	{ .compatible = "amlogic, aml_tas5707_codec", },
	{},
};

static struct platform_driver aml_tas5707_codec_platform_driver = {
	.driver = {
		.name = "tas5707",
		.owner = THIS_MODULE,
		.of_match_table = amlogic_tas5707_codec_dt_match,
	},
	.probe = aml_tas5707_codec_probe,
	.remove = aml_tas5707_codec_remove,
};

static int __init aml_tas5707_modinit(void)
{
	int ret = 0;

	ret = platform_driver_register(&aml_tas5707_codec_platform_driver);
	if (ret != 0)
		pr_err("Failed to register codec tas5707 platform driver\n");
	i2c_add_driver(&tas5707_i2c_driver);
	return ret;
}
static void __exit aml_tas5707_exit(void)
{
	platform_driver_unregister(&aml_tas5707_codec_platform_driver);
	i2c_del_driver(&tas5707_i2c_driver);
}

module_param_array(tas5707_EQ_table,
		uint, &tas5707_EQ_table_length, 0664);
MODULE_PARM_DESC(tas5707_EQ_table,
		"An array of tas5707 EQ param");

module_param_array(tas5707_drc1_table,
		uint, &tas5707_drc1_table_length, 0664);
MODULE_PARM_DESC(tas5707_drc1_table,
		"An array of tas5707 DRC table param");

module_param_array(tas5707_drc1_tko_table,
		uint, &tas5707_drc1_tko_length, 0664);
MODULE_PARM_DESC(tas5707_drc1_tko_table,
		"An array of tas5707 DRC tko table param");

module_init(aml_tas5707_modinit);
module_exit(aml_tas5707_exit);

MODULE_DESCRIPTION("ASoC Tas5707 driver");
MODULE_AUTHOR("AML MM team");
MODULE_LICENSE("GPL");


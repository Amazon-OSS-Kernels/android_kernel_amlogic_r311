#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/tas57xx.h>
#include <linux/amlogic/aml_gpio_consumer.h>

#include "ad87072.h"

/*#define	AD87072_REG_RAM_CHECK*/

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
static void AD87072_early_suspend(struct early_suspend *h);
static void AD87072_late_resume(struct early_suspend *h);
#endif

#define AD87072_RATES (SNDRV_PCM_RATE_32000 | \
		       SNDRV_PCM_RATE_44100 | \
		       SNDRV_PCM_RATE_48000 | \
		       SNDRV_PCM_RATE_64000 | \
		       SNDRV_PCM_RATE_88200 | \
		       SNDRV_PCM_RATE_96000 | \
		       SNDRV_PCM_RATE_176400 | \
		       SNDRV_PCM_RATE_192000)

#define AD87072_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | \
	 SNDRV_PCM_FMTBIT_S24_LE | \
	 SNDRV_PCM_FMTBIT_S32_LE)

static int ad87072_set_EQ_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol);
static int ad87072_get_EQ_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol);
static int ad87072_set_DRC_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol);
static int ad87072_get_DRC_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol);
static const DECLARE_TLV_DB_SCALE(mvol_tlv, -10300, 50, 1);
static const DECLARE_TLV_DB_SCALE(chvol_tlv, -10300, 50, 1);

static const struct snd_kcontrol_new AD87072_snd_controls[] = {
	SOC_SINGLE_TLV("Master Volume", MVOL, 0,
				0xff, 1, mvol_tlv),
	SOC_SINGLE_TLV("Ch1 Volume", C1VOL, 0,
				0xff, 1, chvol_tlv),
	SOC_SINGLE_TLV("Ch2 Volume", C2VOL, 0,
			0xff, 1, chvol_tlv),
	SOC_SINGLE("Ch1 Switch", MUTE, 2, 1, 1),
	SOC_SINGLE("Ch2 Switch", MUTE, 1, 1, 1),
	SOC_SINGLE_BOOL_EXT("Set EQ Enable", 0,
				ad87072_get_EQ_enum, ad87072_set_EQ_enum),
	SOC_SINGLE_BOOL_EXT("Set DRC Enable", 0,
				ad87072_get_DRC_enum, ad87072_set_DRC_enum),
};
static unsigned ad87072_EQ_table_length = 300;
static unsigned ad87072_EQ_table[300] = {
	 0x00, 0x00, 0x00,/*##Channel_1_EQ1_A1*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ1_A2*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ1_B1*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ1_B2*/
	 0x20, 0x00, 0x00,/*##Channel_1_EQ1_A0*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ2_A1*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ2_A2*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ2_B1*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ2_B2*/
	 0x20, 0x00, 0x00,/*##Channel_1_EQ2_A0*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ3_A1*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ3_A2*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ3_B1*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ3_B2*/
	 0x20, 0x00, 0x00,/*##Channel_1_EQ3_A0*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ4_A1*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ4_A2*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ4_B1*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ4_B2*/
	 0x20, 0x00, 0x00,/*##Channel_1_EQ4_A0*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ5_A1*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ5_A2*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ5_B1*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ5_B2*/
	 0x20, 0x00, 0x00,/*##Channel_1_EQ5_A0*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ6_A1*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ6_A2*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ6_B1*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ6_B2*/
	 0x20, 0x00, 0x00,/*##Channel_1_EQ6_A0*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ7_A1*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ7_A2*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ7_B1*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ7_B2*/
	 0x20, 0x00, 0x00,/*##Channel_1_EQ7_A0*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ8_A1*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ8_A2*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ8_B1*/
	 0x00, 0x00, 0x00,/*##Channel_1_EQ8_B2*/
	 0x20, 0x00, 0x00,/*##Channel_1_EQ8_A0*/
	 0x00, 0x00, 0x00,/*##Channel_3_EQ1_A1*/
	 0x00, 0x00, 0x00,/*##Channel_3_EQ1_A2*/
	 0x00, 0x00, 0x00,/*##Channel_3_EQ1_B1*/
	 0x00, 0x00, 0x00,/*##Channel_3_EQ1_B2*/
	 0x20, 0x00, 0x00,/*##Channel_3_EQ1_A0*/
	 0x00, 0x00, 0x00,/*##Channel_3_EQ3_A1*/
	 0x00, 0x00, 0x00,/*##Channel_3_EQ3_A2*/
	 0x00, 0x00, 0x00,/*##Channel_3_EQ3_B1*/
	 0x00, 0x00, 0x00,/*##Channel_3_EQ3_B2*/
	 0x20, 0x00, 0x00,/*##Channel_3_EQ3_A0*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ1_A1*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ1_A2*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ1_B1*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ1_B2*/
	 0x20, 0x00, 0x00,/*##Channel_2_EQ1_A0*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ2_A1*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ2_A2*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ2_B1*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ2_B2*/
	 0x20, 0x00, 0x00,/*##Channel_2_EQ2_A0*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ3_A1*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ3_A2*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ3_B1*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ3_B2*/
	 0x20, 0x00, 0x00,/*##Channel_2_EQ3_A0*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ4_A1*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ4_A2*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ4_B1*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ4_B2*/
	 0x20, 0x00, 0x00,/*##Channel_2_EQ4_A0*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ5_A1*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ5_A2*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ5_B1*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ5_B2*/
	 0x20, 0x00, 0x00,/*##Channel_2_EQ5_A0*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ6_A1*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ6_A2*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ6_B1*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ6_B2*/
	 0x20, 0x00, 0x00,/*##Channel_2_EQ6_A0*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ7_A1*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ7_A2*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ7_B1*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ7_B2*/
	 0x20, 0x00, 0x00,/*##Channel_2_EQ7_A0*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ8_A1*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ8_A2*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ8_B1*/
	 0x00, 0x00, 0x00,/*##Channel_2_EQ8_B2*/
	 0x20, 0x00, 0x00,/*##Channel_2_EQ8_A0*/
	 0x00, 0x00, 0x00,/*##Channel_3_EQ2_A1*/
	 0x00, 0x00, 0x00,/*##Channel_3_EQ2_A2*/
	 0x00, 0x00, 0x00,/*##Channel_3_EQ2_B1*/
	 0x00, 0x00, 0x00,/*##Channel_3_EQ2_B2*/
	 0x20, 0x00, 0x00,/*##Channel_3_EQ2_A0*/
	 0x00, 0x00, 0x00,/*##Channel_3_EQ4_A1*/
	 0x00, 0x00, 0x00,/*##Channel_3_EQ4_A2*/
	 0x00, 0x00, 0x00,/*##Channel_3_EQ4_B1*/
	 0x00, 0x00, 0x00,/*##Channel_3_EQ4_B2*/
	 0x20, 0x00, 0x00,/*##Channel_3_EQ4_A0*/
};

static unsigned ad87072_drc1_table_length = 24;
static u32 ad87072_drc1_table[24] = {
	 0x0c, 0x06, 0xdc,/*##CH1.2_DRC_Attack_threshold*/
	 0x0b, 0x5a, 0xa1,/*##CH1.2_DRC_Release_threshold*/
	 0x20, 0x00, 0x00,/*##CH3_DRC_Attack_threshold*/
	 0x08, 0x00, 0x00,/*##CH3_DRC_Release_threshold*/
	 0x00, 0x00, 0x1a,/*##Noise_Gate_Attack_Level*/
	 0x00, 0x00, 0x53,/*##Noise_Gate_Release_Level*/
	 0x00, 0x80, 0x00,/*##DRC1_Energy_Coefficients*/
	 0x00, 0x20, 0x00,/*##DRC2_Energy_Coefficients*/
};
/* Power-up register defaults */
struct reg_default AD87072_reg_defaults[AD87072_REGISTER_COUNT] = {
	{0x00, 0x00},/*##State_Control_1*/
	{0x01, 0x01},/*##State_Control_2*/
	{0x02, 0x00},/*##State_Control_3*/
	{0x03, 0x05},/*##Master_volume_co*/
	{0x04, 0x12},/*##Channel_1_volume*/
	{0x05, 0x12},/*##Channel_2_volume*/
	{0x06, 0x14},/*##Channel_3_volume*/
	{0x07, 0x10},/*##Bass_tone_boost_*/
	{0x08, 0x10},/*##Treble_tone_boos*/
	{0x09, 0x6a},/*##Bass_management_*/
	{0x0A, 0xb8},/*##State_Control_4*/
	{0x0B, 0x10},/*##Channel_1_config*/
	{0x0C, 0x10},/*##Channel_2_config*/
	{0x0D, 0x00},/*##Channel_3_config*/
	{0x0E, 0x6a},/*##DRC1_limiter_att*/
	{0x0F, 0x6a},/*##DRC2_limiter_att*/
	{0x10, 0x26},/*##Reserved*/
	{0x11, 0x32},/*##State_Control_5*/
	{0x12, 0x01},/*##PVDD_under_volta*/
	{0x13, 0x00},/*##Zero_detection_l*/
	{0x14, 0x7f},/*##Coefficient_RAM_*/
	{0x15, 0x00},/*##Top_8-bits_of_co*/
	{0x16, 0x00},/*##Middle_8-bits_of*/
	{0x17, 0x00},/*##Bottom_8-bits_of*/
	{0x18, 0x00},/*##Top_8-bits_of_co*/
	{0x19, 0x00},/*##Middle_8-bits_of*/
	{0x1A, 0x00},/*##Bottom_8-bits_of*/
	{0x1B, 0x00},/*##Top_8-bits_of_co*/
	{0x1C, 0x00},/*##Middle_8-bits_of*/
	{0x1D, 0x00},/*##Bottom_8-bits_of*/
	{0x1E, 0x00},/*##Top_8-bits_of_co*/
	{0x1F, 0x00},/*##Middle_8-bits_of*/
	{0x20, 0x00},/*##Bottom_8-bits_of*/
	{0x21, 0x20},/*##Top_8-bits_of_co*/
	{0x22, 0x00},/*##Middle_8-bits_of*/
	{0x23, 0x00},/*##Bottom_8-bits_of*/
	{0x24, 0x00},/*##CfRW*/
	{0x25, 0x00},/*##Reserved*/
	{0x26, 0x00},/*##Reserved*/
	{0x27, 0x3f},/*##Reserved*/
	{0x28, 0x00},/*##Reserved*/
	{0x29, 0x00},/*##Reserved*/
	{0x2A, 0x0d},/*##Power_saving_mod*/
	{0x2B, 0x40},/*##Volume_fine_tune*/
	{0x2C, 0xc0},/*##OC_Level*/
	{0x38, 0x0e},/*##Hi-resolution*/
};
static int m_reg_tab[AD87072_REGISTER_COUNT][2] = {
{0x00, 0x00},/*##State_Control_1*/
	{0x01, 0x01},/*##State_Control_2*/
	{0x02, 0x00},/*##State_Control_3*/
	{0x03, 0x05},/*##Master_volume_co*/
	{0x04, 0x12},/*##Channel_1_volume*/
	{0x05, 0x12},/*##Channel_2_volume*/
	{0x06, 0x14},/*##Channel_3_volume*/
	{0x07, 0x10},/*##Bass_tone_boost_*/
	{0x08, 0x10},/*##Treble_tone_boos*/
	{0x09, 0x6a},/*##Bass_management_*/
	{0x0A, 0xb8},/*##State_Control_4*/
	{0x0B, 0x10},/*##Channel_1_config*/
	{0x0C, 0x10},/*##Channel_2_config*/
	{0x0D, 0x00},/*##Channel_3_config*/
	{0x0E, 0x6a},/*##DRC1_limiter_att*/
	{0x0F, 0x6a},/*##DRC2_limiter_att*/
	{0x10, 0x26},/*##Reserved*/
	{0x11, 0x32},/*##State_Control_5*/
	{0x12, 0x01},/*##PVDD_under_volta*/
	{0x13, 0x00},/*##Zero_detection_l*/
	{0x14, 0x7f},/*##Coefficient_RAM_*/
	{0x15, 0x00},/*##Top_8-bits_of_co*/
	{0x16, 0x00},/*##Middle_8-bits_of*/
	{0x17, 0x00},/*##Bottom_8-bits_of*/
	{0x18, 0x00},/*##Top_8-bits_of_co*/
	{0x19, 0x00},/*##Middle_8-bits_of*/
	{0x1A, 0x00},/*##Bottom_8-bits_of*/
	{0x1B, 0x00},/*##Top_8-bits_of_co*/
	{0x1C, 0x00},/*##Middle_8-bits_of*/
	{0x1D, 0x00},/*##Bottom_8-bits_of*/
	{0x1E, 0x00},/*##Top_8-bits_of_co*/
	{0x1F, 0x00},/*##Middle_8-bits_of*/
	{0x20, 0x00},/*##Bottom_8-bits_of*/
	{0x21, 0x20},/*##Top_8-bits_of_co*/
	{0x22, 0x00},/*##Middle_8-bits_of*/
	{0x23, 0x00},/*##Bottom_8-bits_of*/
	{0x24, 0x00},/*##CfRW*/
	{0x25, 0x00},/*##Reserved*/
	{0x26, 0x00},/*##Reserved*/
	{0x27, 0x3f},/*##Reserved*/
	{0x28, 0x00},/*##Reserved*/
	{0x29, 0x00},/*##Reserved*/
	{0x2A, 0x0d},/*##Power_saving_mod*/
	{0x2B, 0x40},/*##Volume_fine_tune*/
	{0x2C, 0xc0},/*##OC_Level*/
	{0x38, 0x0e},/*##Hi-resolution*/
};
static int m_ram_tab[][4] = {
	{0x00, 0x00, 0x00, 0x00},/*##Channel_1_EQ1_A1*/
	{0x01, 0x00, 0x00, 0x00},/*##Channel_1_EQ1_A2*/
	{0x02, 0x00, 0x00, 0x00},/*##Channel_1_EQ1_B1*/
	{0x03, 0x00, 0x00, 0x00},/*##Channel_1_EQ1_B2*/
	{0x04, 0x20, 0x00, 0x00},/*##Channel_1_EQ1_A0*/
	{0x05, 0x00, 0x00, 0x00},/*##Channel_1_EQ2_A1*/
	{0x06, 0x00, 0x00, 0x00},/*##Channel_1_EQ2_A2*/
	{0x07, 0x00, 0x00, 0x00},/*##Channel_1_EQ2_B1*/
	{0x08, 0x00, 0x00, 0x00},/*##Channel_1_EQ2_B2*/
	{0x09, 0x20, 0x00, 0x00},/*##Channel_1_EQ2_A0*/
	{0x0A, 0x00, 0x00, 0x00},/*##Channel_1_EQ3_A1*/
	{0x0B, 0x00, 0x00, 0x00},/*##Channel_1_EQ3_A2*/
	{0x0C, 0x00, 0x00, 0x00},/*##Channel_1_EQ3_B1*/
	{0x0D, 0x00, 0x00, 0x00},/*##Channel_1_EQ3_B2*/
	{0x0E, 0x20, 0x00, 0x00},/*##Channel_1_EQ3_A0*/
	{0x0F, 0x00, 0x00, 0x00},/*##Channel_1_EQ4_A1*/
	{0x10, 0x00, 0x00, 0x00},/*##Channel_1_EQ4_A2*/
	{0x11, 0x00, 0x00, 0x00},/*##Channel_1_EQ4_B1*/
	{0x12, 0x00, 0x00, 0x00},/*##Channel_1_EQ4_B2*/
	{0x13, 0x20, 0x00, 0x00},/*##Channel_1_EQ4_A0*/
	{0x14, 0x00, 0x00, 0x00},/*##Channel_1_EQ5_A1*/
	{0x15, 0x00, 0x00, 0x00},/*##Channel_1_EQ5_A2*/
	{0x16, 0x00, 0x00, 0x00},/*##Channel_1_EQ5_B1*/
	{0x17, 0x00, 0x00, 0x00},/*##Channel_1_EQ5_B2*/
	{0x18, 0x20, 0x00, 0x00},/*##Channel_1_EQ5_A0*/
	{0x19, 0x00, 0x00, 0x00},/*##Channel_1_EQ6_A1*/
	{0x1A, 0x00, 0x00, 0x00},/*##Channel_1_EQ6_A2*/
	{0x1B, 0x00, 0x00, 0x00},/*##Channel_1_EQ6_B1*/
	{0x1C, 0x00, 0x00, 0x00},/*##Channel_1_EQ6_B2*/
	{0x1D, 0x20, 0x00, 0x00},/*##Channel_1_EQ6_A0*/
	{0x1E, 0x00, 0x00, 0x00},/*##Channel_1_EQ7_A1*/
	{0x1F, 0x00, 0x00, 0x00},/*##Channel_1_EQ7_A2*/
	{0x20, 0x00, 0x00, 0x00},/*##Channel_1_EQ7_B1*/
	{0x21, 0x00, 0x00, 0x00},/*##Channel_1_EQ7_B2*/
	{0x22, 0x20, 0x00, 0x00},/*##Channel_1_EQ7_A0*/
	{0x23, 0x00, 0x00, 0x00},/*##Channel_1_EQ8_A1*/
	{0x24, 0x00, 0x00, 0x00},/*##Channel_1_EQ8_A2*/
	{0x25, 0x00, 0x00, 0x00},/*##Channel_1_EQ8_B1*/
	{0x26, 0x00, 0x00, 0x00},/*##Channel_1_EQ8_B2*/
	{0x27, 0x20, 0x00, 0x00},/*##Channel_1_EQ8_A0*/
	{0x28, 0x00, 0x00, 0x00},/*##Channel_1_EQ9_A1*/
	{0x29, 0x00, 0x00, 0x00},/*##Channel_1_EQ9_A2*/
	{0x2A, 0x00, 0x00, 0x00},/*##Channel_1_EQ9_B1*/
	{0x2B, 0x00, 0x00, 0x00},/*##Channel_1_EQ9_B2*/
	{0x2C, 0x20, 0x00, 0x00},/*##Channel_1_EQ9_A0*/
	{0x2D, 0x00, 0x00, 0x00},/*##Channel_1_EQ10_A1*/
	{0x2E, 0x00, 0x00, 0x00},/*##Channel_1_EQ10_A2*/
	{0x2F, 0x00, 0x00, 0x00},/*##Channel_1_EQ10_B1*/
	{0x30, 0x00, 0x00, 0x00},/*##Channel_1_EQ10_B2*/
	{0x31, 0x20, 0x00, 0x00},/*##Channel_1_EQ10_A0*/
	{0x32, 0x00, 0x00, 0x00},/*##Channel_2_EQ1_A1*/
	{0x33, 0x00, 0x00, 0x00},/*##Channel_2_EQ1_A2*/
	{0x34, 0x00, 0x00, 0x00},/*##Channel_2_EQ1_B1*/
	{0x35, 0x00, 0x00, 0x00},/*##Channel_2_EQ1_B2*/
	{0x36, 0x20, 0x00, 0x00},/*##Channel_2_EQ1_A0*/
	{0x37, 0x00, 0x00, 0x00},/*##Channel_2_EQ2_A1*/
	{0x38, 0x00, 0x00, 0x00},/*##Channel_2_EQ2_A2*/
	{0x39, 0x00, 0x00, 0x00},/*##Channel_2_EQ2_B1*/
	{0x3A, 0x00, 0x00, 0x00},/*##Channel_2_EQ2_B2*/
	{0x3B, 0x20, 0x00, 0x00},/*##Channel_2_EQ2_A0*/
	{0x3C, 0x00, 0x00, 0x00},/*##Channel_2_EQ3_A1*/
	{0x3D, 0x00, 0x00, 0x00},/*##Channel_2_EQ3_A2*/
	{0x3E, 0x00, 0x00, 0x00},/*##Channel_2_EQ3_B1*/
	{0x3F, 0x00, 0x00, 0x00},/*##Channel_2_EQ3_B2*/
	{0x40, 0x20, 0x00, 0x00},/*##Channel_2_EQ3_A0*/
	{0x41, 0x00, 0x00, 0x00},/*##Channel_2_EQ4_A1*/
	{0x42, 0x00, 0x00, 0x00},/*##Channel_2_EQ4_A2*/
	{0x43, 0x00, 0x00, 0x00},/*##Channel_2_EQ4_B1*/
	{0x44, 0x00, 0x00, 0x00},/*##Channel_2_EQ4_B2*/
	{0x45, 0x20, 0x00, 0x00},/*##Channel_2_EQ4_A0*/
	{0x46, 0x00, 0x00, 0x00},/*##Channel_2_EQ5_A1*/
	{0x47, 0x00, 0x00, 0x00},/*##Channel_2_EQ5_A2*/
	{0x48, 0x00, 0x00, 0x00},/*##Channel_2_EQ5_B1*/
	{0x49, 0x00, 0x00, 0x00},/*##Channel_2_EQ5_B2*/
	{0x4A, 0x20, 0x00, 0x00},/*##Channel_2_EQ5_A0*/
	{0x4B, 0x00, 0x00, 0x00},/*##Channel_2_EQ6_A1*/
	{0x4C, 0x00, 0x00, 0x00},/*##Channel_2_EQ6_A2*/
	{0x4D, 0x00, 0x00, 0x00},/*##Channel_2_EQ6_B1*/
	{0x4E, 0x00, 0x00, 0x00},/*##Channel_2_EQ6_B2*/
	{0x4F, 0x20, 0x00, 0x00},/*##Channel_2_EQ6_A0*/
	{0x50, 0x00, 0x00, 0x00},/*##Channel_2_EQ7_A1*/
	{0x51, 0x00, 0x00, 0x00},/*##Channel_2_EQ7_A2*/
	{0x52, 0x00, 0x00, 0x00},/*##Channel_2_EQ7_B1*/
	{0x53, 0x00, 0x00, 0x00},/*##Channel_2_EQ7_B2*/
	{0x54, 0x20, 0x00, 0x00},/*##Channel_2_EQ7_A0*/
	{0x55, 0x00, 0x00, 0x00},/*##Channel_2_EQ8_A1*/
	{0x56, 0x00, 0x00, 0x00},/*##Channel_2_EQ8_A2*/
	{0x57, 0x00, 0x00, 0x00},/*##Channel_2_EQ8_B1*/
	{0x58, 0x00, 0x00, 0x00},/*##Channel_2_EQ8_B2*/
	{0x59, 0x20, 0x00, 0x00},/*##Channel_2_EQ8_A0*/
	{0x5A, 0x00, 0x00, 0x00},/*##Channel_2_EQ9_A1*/
	{0x5B, 0x00, 0x00, 0x00},/*##Channel_2_EQ9_A2*/
	{0x5C, 0x00, 0x00, 0x00},/*##Channel_2_EQ9_B1*/
	{0x5D, 0x00, 0x00, 0x00},/*##Channel_2_EQ9_B2*/
	{0x5E, 0x20, 0x00, 0x00},/*##Channel_2_EQ9_A0*/
	{0x5F, 0x00, 0x00, 0x00},/*##Channel_2_EQ10_A1*/
	{0x60, 0x00, 0x00, 0x00},/*##Channel_2_EQ10_A2*/
	{0x61, 0x00, 0x00, 0x00},/*##Channel_2_EQ10_B1*/
	{0x62, 0x00, 0x00, 0x00},/*##Channel_2_EQ10_B2*/
	{0x63, 0x20, 0x00, 0x00},/*##Channel_2_EQ10_A0*/
	{0x64, 0x7f, 0xff, 0xff},/*##Channel-1_Mixer1*/
	{0x65, 0x00, 0x00, 0x00},/*##Channel-1_Mixer2*/
	{0x66, 0x00, 0x00, 0x00},/*##Channel-2_Mixer1*/
	{0x67, 0x7f, 0xff, 0xff},/*##Channel-2_Mixer2*/
	{0x68, 0x20, 0x00, 0x00},/*##POST_DRC_Attack_threshold*/
	{0x69, 0x08, 0x00, 0x00},/*##POST_DRC_Release_threshold*/
	{0x6A, 0x7f, 0xff, 0xff},/*##Channel-1_Prescale*/
	{0x6B, 0x7f, 0xff, 0xff},/*##Channel-2_Prescale*/
	{0x6C, 0x7f, 0xff, 0xff},/*##Channel-1_Postscale*/
	{0x6D, 0x7f, 0xff, 0xff},/*##Channel-2_Postscale*/
	{0x6E, 0x00, 0x80, 0x00},/*##PDRC_Energy_Coefficient*/
	{0x6F, 0x20, 0x00, 0x00},/*##Channel1.2_Power_Clipping*/
	{0x70, 0x20, 0x00, 0x00},/*##Reserved*/
	{0x71, 0x20, 0x00, 0x00},/*##CH1.2_DRC_Attack_threshold*/
	{0x72, 0x08, 0x00, 0x00},/*##CH1.2_DRC_Release_threshold*/
	{0x73, 0x20, 0x00, 0x00},/*##CH3_DRC_Attack_threshold*/
	{0x74, 0x08, 0x00, 0x00},/*##CH3_DRC_Release_threshold*/
	{0x75, 0x00, 0x00, 0x1a},/*##Noise_Gate_Attack_Level*/
	{0x76, 0x00, 0x00, 0x53},/*##Noise_Gate_Release_Level*/
	{0x77, 0x00, 0x80, 0x00},/*##DRC1_Energy_Coefficients*/
	{0x78, 0x00, 0x20, 0x00},/*##DRC2_Energy_Coefficients*/
	{0x79, 0xc7, 0xb6, 0x91},/*##A0_of_SRS_HPF*/
	{0x7A, 0x38, 0x49, 0x6e},/*##A1_of_SRS_HPF*/
	{0x7B, 0x0c, 0x46, 0xf8},/*##B1_of_SRS_HPF*/
	{0x7C, 0x0e, 0x81, 0xb9},/*##A0_of_SRS_LPF*/
	{0x7D, 0xf2, 0x2c, 0x12},/*##A1_of_SRS_LPF*/
	{0x7E, 0x0f, 0xca, 0xbb},/*##B1_of_SRS_LPF*/
};

/* codec private data */
struct AD87072_priv {
	struct regmap *regmap;
	struct snd_soc_codec *codec;
	struct tas57xx_platform_data *pdata;
	enum snd_soc_control_type control_type;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	unsigned char Ch1_vol;
	unsigned char Ch2_vol;
	unsigned char master_vol;

};

#define EQ_TABLE_ADD	0x00
static int ad87072_set_eq_data(struct snd_soc_codec *codec)
{
	int i = 0, j = 0, m = 0;
	u32 *reg_ptr = &ad87072_EQ_table[0];
	for (i = 0; i < 100; i++) {
		for (j = 0; j < 3; j++, m++)
			m_ram_tab[EQ_TABLE_ADD+i][j+1] = reg_ptr[i*3+j];
	}

	for (i = 0; i < 100; i++) {
		snd_soc_write(codec, CFADDR, m_ram_tab[i][0]);
		snd_soc_write(codec, A1CF1, m_ram_tab[i][1]);
		snd_soc_write(codec, A1CF2, m_ram_tab[i][2]);
		snd_soc_write(codec, A1CF3, m_ram_tab[i][3]);
		snd_soc_write(codec, CFUD, 0x01);
	}

	return 0;
}

#define DRC_TABLE_ADD	0x71
static int ad87072_set_drc_data(struct snd_soc_codec *codec)
{
	int i = 0, j = 0, m = 0;
	u32 *reg_ptr = &ad87072_drc1_table[0];

	for (i = 0; i < 8; i++) {
		for (j = 0; j < 3; j++, m++)
			m_ram_tab[DRC_TABLE_ADD + i][j+1] = reg_ptr[i*3+j];
	}

	for (i = DRC_TABLE_ADD; i < AD82586D_DRC_RAM_TABLE_COUNT; i++) {
		snd_soc_write(codec, CFADDR, m_ram_tab[i][0]);
		snd_soc_write(codec, A1CF1, m_ram_tab[i][1]);
		snd_soc_write(codec, A1CF2, m_ram_tab[i][2]);
		snd_soc_write(codec, A1CF3, m_ram_tab[i][3]);
		snd_soc_write(codec, CFUD, 0x01);
	}

	return 0;
}

bool EQ_enum_value_AD8X = 0;
static int ad87072_set_EQ_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	EQ_enum_value_AD8X = ucontrol->value.integer.value[0];

	if (EQ_enum_value_AD8X == 1) {
		u16 reg = snd_soc_read(codec, 0x0a);
		ad87072_set_eq_data(codec);
		reg &= ~(1 << 1);
		snd_soc_write(codec, 0x0a, reg);

	} else {
		u16 reg = snd_soc_read(codec, 0x0a);
		snd_soc_write(codec, 0x0a, reg | (1 << 1));
	}

	return 0;
}
static int ad87072_get_EQ_enum(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = EQ_enum_value_AD8X;
	return 0;
}

bool DRC_enum_value_AD8X = 1;
static int ad87072_set_DRC_enum(struct snd_kcontrol *kcontrol,
				   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	DRC_enum_value_AD8X = ucontrol->value.integer.value[0];

	if (DRC_enum_value_AD8X == 1) {
		u16 reg = snd_soc_read(codec, 0x0a);
		ad87072_set_drc_data(codec);
		reg |= (1 << 5);
		snd_soc_write(codec, 0x0a, reg);
	} else {
		u16 reg = snd_soc_read(codec, 0x0a);
		reg &= ~(1 << 5);
		snd_soc_write(codec, 0x0a, reg);
	}

	return 0;
}
static int ad87072_get_DRC_enum(struct snd_kcontrol *kcontrol,
				    struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = DRC_enum_value_AD8X;
	return 0;
}

static int AD87072_set_dai_sysclk(struct snd_soc_dai *codec_dai,
				  int clk_id, unsigned int freq, int dir)
{
	return 0;
}

static int AD87072_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		break;
	default:
		return 0;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
		break;
	default:
		return 0;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		break;
	case SND_SOC_DAIFMT_NB_IF:
		break;
	default:
		return 0;
	}

	return 0;
}

static int AD87072_hw_params(struct snd_pcm_substream *substream,
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

static int AD87072_set_bias_level(struct snd_soc_codec *codec,
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

static const struct snd_soc_dai_ops AD87072_dai_ops = {
	.hw_params = AD87072_hw_params,
	.set_sysclk = AD87072_set_dai_sysclk,
	.set_fmt = AD87072_set_dai_fmt,
};

static struct snd_soc_dai_driver AD87072_dai = {
	.name = "AD87072",
	.playback = {
		.stream_name = "HIFI Playback",
		.channels_min = 2,
		.channels_max = 8,
		.rates = AD87072_RATES,
		.formats = AD87072_FORMATS,
	},
	.ops = &AD87072_dai_ops,
};

static int AD87072_reg_init(struct snd_soc_codec *codec)
{
	int i = 0;

	for (i = 0; i < AD87072_REGISTER_COUNT; i++) {
		snd_soc_write(codec, m_reg_tab[i][0], m_reg_tab[i][1]);
	};
	return 0;

}

static int AD87072_set_eq_drc(struct snd_soc_codec *codec)
{
	u8 i;

	for (i = 0; i < AD87072_RAM_TABLE_COUNT; i++) {
		snd_soc_write(codec, CFADDR, m_ram_tab[i][0]);
		snd_soc_write(codec, A1CF1, m_ram_tab[i][1]);
		snd_soc_write(codec, A1CF2, m_ram_tab[i][2]);
		snd_soc_write(codec, A1CF3, m_ram_tab[i][3]);
		snd_soc_write(codec, CFUD, 0x01);
	}
	return 0;
}

#ifdef	AD87072_REG_RAM_CHECK
static int AD87072_reg_check(struct snd_soc_codec *codec)
{
	int i = 0;
	int reg_data = 0;

	for (i = 0; i < AD87072_REGISTER_COUNT; i++) {
		reg_data = snd_soc_read(codec, m_reg_tab[i][0]);
		pr_info("AD87072_reg_init  write 0x%x = 0x%x\n",
				m_reg_tab[i][0], reg_data);
	};
	return 0;
}

static int AD87072_eqram_check(struct snd_soc_codec *codec)
{
	int i = 0;
	int H8_data = 0, M8_data = 0, L8_data = 0;

	for (i = 0; i < AD87072_RAM_TABLE_COUNT; i++) {
		snd_soc_write(codec, CFADDR, m_ram_tab[i][0]);
		/* write ram addr*/
		snd_soc_write(codec, CFUD, 0x04);
		/*write read ram cmd*/

		H8_data = snd_soc_read(codec, A1CF1);
		M8_data = snd_soc_read(codec, A1CF2);
		L8_data = snd_soc_read(codec, A1CF3);
		pr_info("AD87072_set_eq_drc ram1  write 0x%x = 0x%x , 0x%x , 0x%x\n",
				m_ram_tab[i][0], H8_data, M8_data, L8_data);
	};
	return 0;
}
#endif

static int reset_AD87072_GPIO(struct snd_soc_codec *codec)
{
	struct AD87072_priv *AD87072 = snd_soc_codec_get_drvdata(codec);
	struct tas57xx_platform_data *pdata = AD87072->pdata;
	dev_info(codec->dev, "reset_AD87072  init!pdata->reset_pin:%d\n",
				pdata->reset_pin);

	if (pdata->reset_pin < 0)
		return 0;

	gpio_direction_output(AD87072->pdata->reset_pin, GPIOF_OUT_INIT_LOW);

	/* SEPC requires a minimum of 10 ms */
	mdelay(20);
	gpio_direction_output(AD87072->pdata->reset_pin, GPIOF_OUT_INIT_HIGH);
	/* normal operation after delay 300 */
	mdelay(300);

	return 0;
}

static int AD87072_init(struct snd_soc_codec *codec)
{
	reset_AD87072_GPIO(codec);
	/* normal operation*/
	mdelay(10);
	dev_info(codec->dev, "AD87072_init!\n");
	pr_err("AD87072_init!\n");
	snd_soc_write(codec, 0x02, 0x0f);		/* mute*/
	/* amp register init*/
	AD87072_reg_init(codec);

	snd_soc_write(codec, 0x02, 0x0f);		/* mute*/
	/*eq and drc init*/
	AD87072_set_eq_drc(codec);

#ifdef	AD87072_REG_RAM_CHECK
	AD87072_reg_check(codec);
	AD87072_eqram_check(codec);
#endif

	/*unmute,default power-on is mute.*/
	snd_soc_write(codec, 0x02, 0x00);		/*unmute*/

	return 0;
}

static int AD87072_probe(struct snd_soc_codec *codec)
{
	int ret = 0;
	struct AD87072_priv *AD87072 = snd_soc_codec_get_drvdata(codec);
	struct tas57xx_platform_data *pdata = dev_get_platdata(codec->dev);
#ifdef CONFIG_HAS_EARLYSUSPEND
	AD87072->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	AD87072->early_suspend.suspend = AD87072_early_suspend;
	AD87072->early_suspend.resume = AD87072_late_resume;
	AD87072->early_suspend.param = codec;
	register_early_suspend(&(AD87072->early_suspend));
#endif
	AD87072->pdata = pdata;
	ret = snd_soc_codec_set_cache_io(codec, 8, 8, SND_SOC_I2C);
	if (ret < 0) {
		dev_err(codec->dev, "fail to reset audio (%d)\n", ret);
		return 0;
	}
	AD87072_init(codec);

	return 0;
}

static int AD87072_remove(struct snd_soc_codec *codec)
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct AD87072_priv *AD87072 = snd_soc_codec_get_drvdata(codec);

	unregister_early_suspend(&(AD87072->early_suspend));
#endif
	return 0;
}

#ifdef CONFIG_PM
static int AD87072_suspend(struct snd_soc_codec *codec)
{
	dev_info(codec->dev, "AD87072_suspend!\n");

	return 0;
}

static int AD87072_resume(struct snd_soc_codec *codec)
{
	dev_info(codec->dev, "AD87072_resume!\n");
	AD87072_init(codec);

	return 0;
}
#else
#define AD87072_suspend NULL
#define AD87072_resume NULL
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void AD87072_early_suspend(struct early_suspend *h)
{
}

static void AD87072_late_resume(struct early_suspend *h)
{
}
#endif

static const struct snd_soc_dapm_widget AD87072_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("DAC", "HIFI Playback", SND_SOC_NOPM, 0, 0),
};

static const struct snd_soc_codec_driver soc_codec_dev_AD87072 = {
	.probe = AD87072_probe,
	.remove = AD87072_remove,
	.suspend = AD87072_suspend,
	.resume = AD87072_resume,
	.set_bias_level = AD87072_set_bias_level,
	.controls = AD87072_snd_controls,
	.num_controls = ARRAY_SIZE(AD87072_snd_controls),
	.dapm_widgets = AD87072_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(AD87072_dapm_widgets),
};

static const struct regmap_config AD87072_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = AD87072_REGISTER_COUNT,
	.reg_defaults = AD87072_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(AD87072_reg_defaults),
	.cache_type = REGCACHE_RBTREE,
};

static int AD87072_i2c_probe(struct i2c_client *i2c,
			     const struct i2c_device_id *id)
{
	struct AD87072_priv *AD87072;
	int ret;

	AD87072 = devm_kzalloc(&i2c->dev, sizeof(struct AD87072_priv),
			       GFP_KERNEL);
	if (!AD87072)
		return -ENOMEM;

	AD87072->regmap = devm_regmap_init_i2c(i2c, &AD87072_regmap);
	if (IS_ERR(AD87072->regmap)) {
		ret = PTR_ERR(AD87072->regmap);
		dev_err(&i2c->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
	}

	i2c_set_clientdata(i2c, AD87072);
	AD87072->control_type = SND_SOC_I2C;

	ret = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_AD87072,
				     &AD87072_dai, 1);
	if (ret != 0)
		dev_err(&i2c->dev, "Failed to register codec (%d)\n", ret);

	return ret;
}

static int AD87072_i2c_remove(struct i2c_client *client)
{
	snd_soc_unregister_codec(&client->dev);

	return 0;
}

static const struct i2c_device_id AD87072_i2c_id[] = {
	{ "AD87072", 0 },
	{}
};

static const struct of_device_id AD87072_of_id[] = {
	{ .compatible = "ESMT, AD87072", },
	{ /* senitel */ }
};

static int aml_AD87072_codec_probe(struct platform_device *pdev)
{
	return 0;
}

static int aml_AD87072_codec_remove(struct platform_device *pdev)
{
	return 0;
}
static struct i2c_driver AD87072_i2c_driver = {
	.driver = {
		.name = "AD87072",
		.of_match_table = AD87072_of_id,
		.owner = THIS_MODULE,
	},
	.probe = AD87072_i2c_probe,
	.remove = AD87072_i2c_remove,
	.id_table = AD87072_i2c_id,
};

static struct platform_driver aml_AD87072_codec_platform_driver = {
	.driver = {
		.name = "AD87072",
		.owner = THIS_MODULE,
		.of_match_table = AD87072_of_id,
	},
	.probe = aml_AD87072_codec_probe,
	.remove = aml_AD87072_codec_remove,
};

static int __init aml_AD87072_modinit(void)
{
	int ret = 0;
	ret = platform_driver_register(&aml_AD87072_codec_platform_driver);
	if (ret != 0)
		pr_err("Failed to register codec tas5717 platform driver\n");
	i2c_add_driver(&AD87072_i2c_driver);
	return ret;
}

static void __exit aml_AD87072_exit(void)
{
	platform_driver_unregister(&aml_AD87072_codec_platform_driver);
	i2c_del_driver(&AD87072_i2c_driver);
}

module_param_array(ad87072_EQ_table, uint, &ad87072_EQ_table_length, 0664);
MODULE_PARM_DESC(ad87072_EQ_table, "An array of ad87072 EQ param");

module_param_array(ad87072_drc1_table, uint, &ad87072_drc1_table_length, 0664);
MODULE_PARM_DESC(ad87072_drc1_table, "An array of ad87072 DRC table param");

module_init(aml_AD87072_modinit);
module_exit(aml_AD87072_exit);

MODULE_DESCRIPTION("ASoC AD87072 driver");
MODULE_AUTHOR("AML MM team");
MODULE_LICENSE("GPL");

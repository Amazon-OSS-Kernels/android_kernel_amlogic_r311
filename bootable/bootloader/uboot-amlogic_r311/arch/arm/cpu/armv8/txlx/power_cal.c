
/*
 * arch/arm/cpu/armv8/xx/power_cal.c
 *
 * Copyright (C) 2016 Amlogic, Inc. All rights reserved.
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <common.h>
#include <asm/arch/gpio.h>
#include <asm/arch/secure_apb.h>
#include <asm/arch/io.h>
#include <asm/saradc.h>
#include <asm/arch/mailbox.h>

#define P_EE_TIMER_E		(*((volatile unsigned *)(0xffd00000 + (0x3c62 << 2))))
#define vcck_adc_channel	0x4
#define ee_adc_channel		0x5
#define default_ref_val		1800
#define ADC_SAMPLE_TIMES 10
#define ADC_SAMPLE_DELAY (10*1000)
#define VOLTAGE_RANGE_DIFF_MAX    (30)    //mV
#define VOLTAGE_RANGE_DIFF_MIN    (-30)    //mV
#define VOLTAGE_DIFF    (5)    //mV

extern const int pwm_cal_voltage_table[][2];
extern const int pwm_cal_voltage_table_ee[][2];
extern int pwm_cal_voltage_table_size;
extern int pwm_cal_voltage_table_ee_size;


enum pwm_id {
    pwm_vcck = 0,
    pwm_ee,
};

unsigned int _get_time(void)
{
	return P_EE_TIMER_E;
}

void _udelay_(unsigned int us)
{
	unsigned int t0 = _get_time();

	while (_get_time() - t0 <= us)
		;
}

int32_t aml_delt_get(int adc_val, unsigned int voltage)
{
	unsigned int adc_volt;
	int32_t delt;
	int32_t div = 10;	/*10mv is min step*/

	if (adc_val != -1) {
		adc_volt = default_ref_val*adc_val/1024;
		printf("aml pwm cal adc_val = %x, adc_voltage = %d, def_voltage = %d\n",
				adc_val, adc_volt, voltage);
	} else {
		adc_volt = voltage;
		printf("warning:aml pwm cal adc get voltage error\n");
		return 0;
	}
	delt = voltage - adc_volt;
	delt = delt / div;

	return delt;
}

void aml_set_voltage(unsigned int id, unsigned int voltage, int delt)
{
	int to;

	switch (id) {
	case pwm_vcck:
		for (to = 0; to < pwm_cal_voltage_table_size; to++) {
			if (pwm_cal_voltage_table[to][1] >= voltage) {
				break;
			}
		}
		to +=delt;
		if (to >= pwm_cal_voltage_table_size) {
			to = pwm_cal_voltage_table_size - 1;
		}
		if (to < 0) {
			printf("warning: aml pwm cal vcck volt set min\n");
			to = 0;
		}
		/*vcck volt set by dvfs and avs*/
		/*
		writel(pwm_voltage_table[to][0], PWM_PWM_A_ADRESS);
		*_udelay_(200);
		*/
		break;

	case pwm_ee:
		for (to = 0; to < pwm_cal_voltage_table_ee_size; to++) {
			if (pwm_cal_voltage_table_ee[to][1] >= voltage) {
				break;
				}
		}
		to +=delt;
		if (to >= pwm_cal_voltage_table_ee_size) {
			to = pwm_cal_voltage_table_ee_size - 1;
		}
		if (to < 0) {
			printf("warning: aml pwm cal ee volt set min\n");
			to = 0;
		}
		printf("aml pwm cal before ee_address: %x, ee_voltage: %x\n",
				AO_PWM_PWM_B, readl(AO_PWM_PWM_B));
		writel(pwm_cal_voltage_table_ee[to][0],AO_PWM_PWM_B);
		_udelay_(1000);
		printf("aml pwm cal after ee_address: %x, ee_voltage: %x\n",
				AO_PWM_PWM_B, readl(AO_PWM_PWM_B));
		break;
	default:
		break;
	}
	_udelay_(200);
}

static void aml_set_vref(int enable)
{
	if (1 == enable) {
		if ((readl(AO_SEC_SD_CFG12) >> 19) & 0x1f) { /*adc VREF*/
			writel(((readl(AO_SAR_ADC_REG13)) & (~(0x3f << 8)))
			| (((readl(AO_SEC_SD_CFG12) >> 19) & 0x1f) << 9),
			AO_SAR_ADC_REG13);
			writel((readl(AO_SAR_ADC_REG11) & (~0x1)), AO_SAR_ADC_REG11);
		} else {
			printf("aml pwm cal no use efuse vref\n");
		}
		/*debug
		*printf("aml pwm cali reg11: %x\n", readl(AO_SAR_ADC_REG11));
		*printf("aml pwm cali reg13: %x\n", readl(AO_SAR_ADC_REG13));
		*/
	} else {
		writel((readl(AO_SAR_ADC_REG11) | 0x1), AO_SAR_ADC_REG11);
		/*debug
		*printf("aml pwm cali reg11: %x\n", readl(AO_SAR_ADC_REG11));
		*printf("aml pwm cali reg13: %x\n", readl(AO_SAR_ADC_REG13));
		*/
	}
}

int comparator(const void *p, const void *q)
{
	int l = *(const int *)p;
	int r = *(const int *)q;
	return (l-r);
}

static int aml_adc_get(int channel, int voltage)
{
	int adc_array[ADC_SAMPLE_TIMES];
	int sum = 0, i = 0, rmfirst = 1;
	int average = 0;
	int adc_volt = 0;
	int adc_size = 0;

	/*get adc value*/
	aml_set_vref(1);
	for (i = 0; i < ADC_SAMPLE_TIMES; i++) {
		adc_array[i] = get_adc_sample_gxbb(channel);
		adc_volt = default_ref_val * adc_array[i] / 1024;
		if (adc_volt > (voltage + VOLTAGE_RANGE_DIFF_MAX))
			adc_array[i] = 0;
		if (adc_volt < (voltage + VOLTAGE_RANGE_DIFF_MIN))
			adc_array[i] = 0;
		_udelay_(ADC_SAMPLE_DELAY);
		/*
		*printf("array adc: %x\n", adc_array[i]);
		*/
	}
	aml_set_vref(0);
	/*sort adc value*/
	qsort((void*)adc_array, ADC_SAMPLE_TIMES, sizeof(adc_array[0]), comparator);
	/*remove MAX and MIN value and averaging*/
	for (i = 0; i < ADC_SAMPLE_TIMES - 1; i++) {
		if (0 != adc_array[i]) {
			if (1 == rmfirst) {
				rmfirst = 0;
			} else {
				sum += adc_array[i];
				adc_size++;
			}
		}
		/* for debug
		*printf("averaging array adc: %x, sum: %x\n", adc_array[i], sum);
		*/
	}

	if (0 == sum)
		return -1;

	average = sum / adc_size;
	return average;
}

int get_hw_subid(void)
{
	unsigned int hw_subid = 0;
	setbits_le32(P_PREG_PAD_GPIO3_O,1<<10);
	setbits_le32(P_PREG_PAD_GPIO3_EN_N,1<<10);
	hw_subid = (readl(P_PREG_PAD_GPIO3_I) & 1<<10) >> 10;
	/*
		hw_subid=1  ddr4
		hw_subid=0  ddr3
	*/
	return hw_subid;
}

void aml_cal_pwm(unsigned int ee_voltage, unsigned int vcck_voltage)
{
	int ee_delt = 0, vcck_delt = 0;
	int ee_val, vcck_val;
	/*txlx vcck ch4,vddee ch5*/
	vcck_val= aml_adc_get(vcck_adc_channel, CONFIG_VCCK_INIT_VOLTAGE);
	if (-1 != vcck_val)
		vcck_delt = aml_delt_get(vcck_val, CONFIG_VCCK_INIT_VOLTAGE);
	if (0 != vcck_delt)
		aml_set_voltage(pwm_vcck, CONFIG_VCCK_INIT_VOLTAGE, vcck_delt);
#if defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC)
	ee_val = aml_adc_get(ee_adc_channel, CONFIG_VDDEE_INIT_VOLTAGE_DDR3);
	if (-1 != ee_val)
		ee_delt = aml_delt_get(ee_val, CONFIG_VDDEE_INIT_VOLTAGE_DDR3 + VOLTAGE_DIFF);
	if (0 != ee_delt)
		aml_set_voltage(pwm_ee, CONFIG_VDDEE_INIT_VOLTAGE_DDR3, ee_delt);
	printf("aml board pwm vcckdelt : %x, eedelt: %x\n", vcck_delt, ee_delt);
#else
	if (get_hw_subid() == 1) {
		ee_val = aml_adc_get(ee_adc_channel, CONFIG_VDDEE_INIT_VOLTAGE);
		if (-1 != ee_val)
			ee_delt = aml_delt_get(ee_val, CONFIG_VDDEE_INIT_VOLTAGE + VOLTAGE_DIFF);
		if (0 != ee_delt)
			aml_set_voltage(pwm_ee, CONFIG_VDDEE_INIT_VOLTAGE, ee_delt);
		printf("aml board pwm vcckdelt: %x, eedelt: %x\n", vcck_delt, ee_delt);
	}else{
		ee_val = aml_adc_get(ee_adc_channel, CONFIG_VDDEE_INIT_VOLTAGE_DDR3);
		if (-1 != ee_val)
			ee_delt = aml_delt_get(ee_val, CONFIG_VDDEE_INIT_VOLTAGE_DDR3 + VOLTAGE_DIFF);
		if (0 != ee_delt)
			aml_set_voltage(pwm_ee, CONFIG_VDDEE_INIT_VOLTAGE_DDR3, ee_delt);
		printf("aml board pwm vcckdelt: %x, eedelt: %x\n", vcck_delt, ee_delt);
	}
#endif
}


void aml_pwm_cal_init(int mode)
{
	printf("aml pwm cal init\n");
	saradc_enable();
#if defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC)
	aml_cal_pwm(CONFIG_VDDEE_INIT_VOLTAGE_DDR3, CONFIG_VCCK_INIT_VOLTAGE);
#else
	if (get_hw_subid() == 1) {
		aml_cal_pwm(CONFIG_VDDEE_INIT_VOLTAGE, CONFIG_VCCK_INIT_VOLTAGE);
	}else{
		aml_cal_pwm(CONFIG_VDDEE_INIT_VOLTAGE_DDR3, CONFIG_VCCK_INIT_VOLTAGE);
	}
#endif
	saradc_disable();
}

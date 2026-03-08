/*
 * Copyright C 2011 by Amlogic, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *
 *
 */
#include <asm/arch/io.h>
#include <asm/cpu_id.h>
#include <common.h>
#include <config.h>
#include <command.h>
#include <amlogic/aml_irblaster.h>
#include <asm/arch/secure_apb.h>

static struct aml_irblaster_drv_s irblaster_drviver;
const char *protocol_name[] = {
	"NEC",
};

/*NEC key value*/
#define NEC_HEADER	9000
#define NEC_IDLE	4500
const static int  NEC_END[] = {
	560,
	560
};
const static int NEC_ONE[] = {
	560,
	1690
};
const static int NEC_ZERO[] = {
	560,
	560
};

static void get_nec_data(unsigned int value)
{
	struct aml_irblaster_drv_s *drv = aml_irblaster_get_driver();
	unsigned int num = 0;
	int index;
	unsigned int *pdata = drv->windows;
	pdata[num++] = NEC_HEADER;
	pdata[num++] = NEC_IDLE;
	/*low bit first*/
	for (index = 0; index < 32; index++) {
		if (value & (0x01 << index)) {
			pdata[num++] = NEC_ONE[0];
			pdata[num++] = NEC_ONE[1];
		} else {
			pdata[num++] = NEC_ZERO[0];
			pdata[num++] = NEC_ZERO[1];
		}
	}
	pdata[num++] = NEC_END[0];
	pdata[num++] = NEC_END[1];
	drv->windows_num = num;
	drv->sendvalue = value;
}

static int send_bit(unsigned int hightime, unsigned int lowtime,
	unsigned int cycle)
{
	unsigned int count_delay;
	uint32_t val;
	int n = 0;
	int tb[3] = {
		1, 10, 100
	};

	/*
	AO_IR_BLASTER_ADDR2
	bit12: output level(or modulation enable/disable:1=enable)
	bit[11:10]: Timebase :
				00=1us
				01=10us
				10=100us
				11=Modulator clock
	bit[9:0]: Count of timebase units to delay
	*/

	count_delay = (((hightime + cycle/2) / cycle) - 1) & 0x3ff;
	val = (0x10000 | (1 << 12)) | (3 << 10) | (count_delay << 0);
	writel(val, AO_IR_BLASTER_ADDR2);

	/*
	lowtime<1024,n=0,timebase=1us
	1024<=lowtime<10240,n=1,timebase=10us
	10240<=lowtime,n=2,timebase=100us
	*/
	n = lowtime >> 10;
	if (n > 0 && n < 10)
		n = 1;
	else if (n >= 10)
		n = 2;
	lowtime = (lowtime + (tb[n] >> 1))/tb[n];
	count_delay = (lowtime-1) & 0x3ff;
	val = (0x10000 | (0 << 12)) |
		(n << 10) | (count_delay << 0);
	writel(val, AO_IR_BLASTER_ADDR2);
	return 0;
}

static void send_frame(void)
{
	int i;
	int exp = 0x00;
	unsigned int* pdata;
	unsigned int high_ct, low_ct;
	struct aml_irblaster_drv_s *drv = aml_irblaster_get_driver();
	unsigned int consumerir_cycle = 1000 / (drv->frequency / 1000);

	/*reset*/
	writel(readl(AO_RTI_GEN_CNTL_REG0) | (1 << 23), AO_IR_BLASTER_ADDR2);
	udelay(2);
	writel(readl(AO_RTI_GEN_CNTL_REG0) & ~(1 << 23), AO_IR_BLASTER_ADDR2);

	/*
	1. disable ir blaster
	2. set the modulator_tb = 2'10; mpeg_1uS_tick 1us
	*/
	writel((1 << 2) | (2 << 12) | (1<<2), AO_IR_BLASTER_ADDR0);
	/*
	1. set mod_high_count = 13
	2. set mod_low_count = 13
	3. 60khz 8, 38k-13us, 12
	*/
	high_ct = consumerir_cycle * drv->dutycycle/100;
	low_ct = consumerir_cycle - high_ct;
	writel(((high_ct - 1) << 16) | ((low_ct - 1) << 0),
		AO_IR_BLASTER_ADDR1);

	/* Setting this bit to 1 initializes the output to be high.*/
	writel(readl(AO_IR_BLASTER_ADDR0) & ~(1 << 2), AO_IR_BLASTER_ADDR0);

	/*enable irblaster*/
	writel(readl(AO_IR_BLASTER_ADDR0) | (1 << 0), AO_IR_BLASTER_ADDR0);
#define SEND_BIT_NUM 64
	exp = drv->windows_num / SEND_BIT_NUM;
	pdata = drv->windows;

	while (exp) {
		for (i = 0; i < SEND_BIT_NUM/2; i++) {
			send_bit(*pdata, *(pdata+1), consumerir_cycle);
			pdata += 2;
		}
		while (!(readl(AO_IR_BLASTER_ADDR0) & (1<<24))) ;
		while (readl(AO_IR_BLASTER_ADDR0) & (1<<26)) ;
		/*reset*/
		writel(readl(AO_RTI_GEN_CNTL_REG0) | (1 << 23),
			AO_RTI_GEN_CNTL_REG0);
		udelay(2);
		/*reset*/
		writel(readl(AO_RTI_GEN_CNTL_REG0) & ~(1 << 23),
			AO_RTI_GEN_CNTL_REG0);
		exp--;
	}
	exp = (drv->windows_num % SEND_BIT_NUM) & (~(1));
	for (i = 0; i < exp; ) {
		send_bit(*pdata, *(pdata+1), consumerir_cycle);
		pdata += 2;
		i += 2;
	}
}

#if defined(UBOOT_TARGET_PRODUCT_NAME_abc123) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC)
extern int gpio_lookup_name(const char *name, struct udevice **devp,
		     unsigned int *offsetp, unsigned int *gpiop);

extern int gpio_request(unsigned gpio, const char *label);

extern int gpio_direction_output(unsigned gpio, int value);


void aml_irblaster_enable(int enabled) {
    unsigned int gpioz_12;
/*
	config GPIOZ_12 as output
*/
	gpio_lookup_name("gpioz_12", NULL, NULL, &gpioz_12);
	gpio_request(gpioz_12, "cmd_gpio");
	gpio_direction_output(gpioz_12, enabled);
#if defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC)
/*
	config GPIOZ_9 as output
*/
	unsigned int gpioz_9;
	gpio_lookup_name("gpioz_9", NULL, NULL, &gpioz_9);
	gpio_request(gpioz_9, "cmd_gpio");
	gpio_direction_output(gpioz_9, enabled);
	/*
        config GPIOZ_11 as output
*/
        unsigned int gpioz_11;
        gpio_lookup_name("gpioz_11", NULL, NULL, &gpioz_11);
        gpio_request(gpioz_11, "cmd_gpio");
        gpio_direction_output(gpioz_11, enabled);
	printf("GPIOZ_9 GPIOZ_11 high-2\n");

#endif
}


void aml_irblaster_pinmux_config(void)
{
	int val;
/*
	GPIOAO_2    UART_CTS_AO_A   AO_reg[10]  UART_CTS_AO_B   AO_reg[8]   REMOTE_OUTPUT   AO_reg[28]  PWM_AO_D    AO_reg2[10]
	config GPIOAO_2 as REMOTE_OUTPUT
*/
	val = (readl(AO_RTI_PIN_MUX_REG) & ~(1<<10 | 1<<8)) | 1 << 28;
	writel(val, AO_RTI_PIN_MUX_REG);


	val = (readl(AO_RTI_PIN_MUX_REG2) & (~1 << 10));
	writel(val, AO_RTI_PIN_MUX_REG2);


/*
	Config GPIOZ_12 As GPIO. enable IR blaster Power, if High
*/
	clrbits_le32(P_PERIPHS_PIN_MUX_4 , (1<<8));
	clrbits_le32(P_PERIPHS_PIN_MUX_3 , (1<<18)|(1<<21)| (1<<5));
	clrbits_le32(P_PERIPHS_PIN_MUX_10, (1<<18)|(1<<16));
#if defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC)
/*
	Config GPIOZ_9 As GPIO. enable IR blaster Power, if High
*/
	clrbits_le32(P_PERIPHS_PIN_MUX_4 , (1<<11));
	clrbits_le32(P_PERIPHS_PIN_MUX_10, (1<<21)|(1<<24)|(1<<26));

/*
        Config GPIOZ_11 As GPIO. enable IR blaster Power, if High
*/
        clrbits_le32(P_PERIPHS_PIN_MUX_4 , (1<<9));
        clrbits_le32(P_PERIPHS_PIN_MUX_10, (1<<19));
	clrbits_le32(P_PERIPHS_PIN_MUX_3 , (1<<6));
	printf("GPIOZ_9 GPIOZ_11 high-1\n");


#endif
	printf("txlx config irblaster pinmux well ON abc123\n");
}
#else
extern void irblaster_pinmux_config(void);
#endif

static int open(void)
{
	struct aml_irblaster_drv_s *drv = aml_irblaster_get_driver();

	drv->protocol = 0; /*default NEC*/
	drv->frequency = 38000; /*freq 38k*/

	/*pinmux*/
#if defined(UBOOT_TARGET_PRODUCT_NAME_abc123) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC)
	aml_irblaster_pinmux_config();

/*
	enable irblaster
*/
	aml_irblaster_enable(1);
#else
	irblaster_pinmux_config();
#endif

	drv->openflag = 1;
	return 0;
}

static int close(void)
{
#if defined(UBOOT_TARGET_PRODUCT_NAME_abc123) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC)
/*
	disable irblaster
*/
	aml_irblaster_enable(0);
#endif
	return 0;
}

static int send(unsigned int value)
{
	get_nec_data(value);
	send_frame();
	return 0;
}

static void print_windows(void)
{
	struct aml_irblaster_drv_s *drv = aml_irblaster_get_driver();
	int n;
	unsigned int *pdata;

	pdata = drv->windows;
	n = drv->windows_num;
	printf ("sendvalue:%u\n", drv->sendvalue);
	printf ("windows_number:%d\n", n);
	printf ("windows:\n");
	while (n--)
		printf ("%d\n", *pdata++);
}

static int setprotocol(char *name)
{
	struct aml_irblaster_drv_s *drv = aml_irblaster_get_driver();
	int p = 0;
	int size = sizeof(protocol_name) / sizeof(protocol_name[0]);

	for (p = 0; p < size; p++) {
		if (!strcmp(protocol_name[p], name)) {
			drv->protocol = p;
			return 0;
		}
	}
	return CMD_RET_USAGE;
}

static const char *getprocotol(void)
{
	struct aml_irblaster_drv_s *drv = aml_irblaster_get_driver();
	int size = sizeof(protocol_name) / sizeof(protocol_name[0]);

	if (drv->protocol >= size)
		return NULL;
	return protocol_name[drv->protocol];
}

static int setfrequency(unsigned int freq)
{
	struct aml_irblaster_drv_s *drv = aml_irblaster_get_driver();

	if (freq < 20000 || freq > 60000) {
		printf("20000<freq<60000\n");
		return CMD_RET_USAGE;
	}
	drv->frequency = freq;
	return 0;
}

static unsigned int getfrequency(void)
{
	struct aml_irblaster_drv_s *drv = aml_irblaster_get_driver();

	return drv->frequency;
}

unsigned char ci_nec_fe01[] = {
	0x01,
	0x02,
	0x03,
	0x04,
	0x05,
	0x06,
	0x07,
	0x08,
	0x09,
	0x0a,
	0x1F,
	0x15,
	0x16,
	0x0c,
	0x0d,
	0x0e,
	0x0f,
	0x11,
	0x1c,
	0x1b,
	0x19,
	0x1a,
	0x1d,
	0x17,
	0x49,
	0x43,
	0x12,
	0x14,
	0x18,
	0x59,
	0x5a,
	0x42,
	0x44,
	0x1e,
	0x4b,
	0x58,
	0x46,
	0x40,
	0x38,
	0x57,
	0x5b,
	0x54,
	0x4c,
	0x4e,
	0x55,
	0x53,
	0x52,
	0x39,
	0x41,
	0x0b,
	0x00,
	0x13
};

static int loop_test(unsigned int times)
{
	unsigned int framecode;
	int n, cnt = 0;
	int size = ARRAY_SIZE(ci_nec_fe01);

	printf("test loop...\n");
	for ( ; times--; ) {
		printf("times=%d\n", cnt++);
		for (n = 0; n < size; n++) {
			framecode = 0xfe01 | ((~ci_nec_fe01[n] & 0xff) << 24) |
				(ci_nec_fe01[n] << 16);
			printf("0x%x\n", framecode);
			get_nec_data(framecode);
			send_frame();
			mdelay(500);
		}
	}
	return 0;
}

int read_reg(volatile unsigned int *addr, unsigned int length)
{
	int n;
	int value;

	printf("read_reg\n");
	for (n = 0; n < length; n++) {
		value = readl(addr);
		printf("0x%p=0x%x\n", addr, value);
		++addr;
	}
	return 0;
}

int write_reg(volatile unsigned int *addr, unsigned int value)
{
	writel(value, addr);
	printf("0x%p=0x%x", addr, value);
	return 0;
}

static struct aml_irblaster_drv_s irblaster_drviver = {
	.frequency = 38000,
	.dutycycle = 50,
	.protocol = 0,
	.windows_num = 0,
	.openflag = 0,
	.open = open,
	.close = close,
	.send = send,
	.setprotocol = setprotocol,
	.getprocotol = getprocotol,
	.setfrequency = setfrequency,
	.getfrequency = getfrequency,
	.test = loop_test,
	.print_windows = print_windows,
	.read_reg = read_reg,
	.write_reg = write_reg,
};

struct aml_irblaster_drv_s *aml_irblaster_get_driver(void)
{
	return &irblaster_drviver;
}

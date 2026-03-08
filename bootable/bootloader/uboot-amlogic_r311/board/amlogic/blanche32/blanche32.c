/*
 * board/amlogic/blanche32/blanche32.c
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/
#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <environment.h>
#include <fdt_support.h>
#include <libfdt.h>
#include <asm/cpu_id.h>
#include <asm/arch/bl31_apis.h>
#include <asm/reboot.h>
#ifdef CONFIG_SYS_I2C_AML
#include <aml_i2c.h>
#include <asm/arch/secure_apb.h>
#endif
#ifdef CONFIG_AML_VPU
#include <vpu.h>
#endif
#include <vpp.h>
#ifdef CONFIG_AML_V2_FACTORY_BURN
#include <amlogic/aml_v2_burning.h>
#endif// #ifdef CONFIG_AML_V2_FACTORY_BURN
#ifdef CONFIG_AML_HDMITX20
#include <amlogic/hdmi.h>
#endif
#ifdef CONFIG_AML_LCD
#include <amlogic/aml_lcd.h>
#endif
#include <asm/arch/eth_setup.h>
#include <phy.h>
#include <asm-generic/gpio.h>
#ifdef CONFIG_IDME
#include <idme.h>

#endif
#ifdef DTB_BIND_KERNEL
#include "storage.h"
#endif
#include <asm/arch/mailbox.h>

#ifdef CONFIG_AMZN_SILENT_OTA
#include "amzn_silent_ota.h"
#endif

DECLARE_GLOBAL_DATA_PTR;

//new static eth setup
struct eth_board_socket*  eth_board_skt;

int board_id_type_check(void);

int serial_set_pin_port(unsigned long port_base)
{
    //UART in "Always On Module"
    //GPIOAO_0==tx,GPIOAO_1==rx
    //setbits_le32(P_AO_RTI_PIN_MUX_REG,3<<11);
    return 0;
}

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_1_SIZE;
	return 0;
}

/* secondary_boot_func
 * this function should be write with asm, here, is is only for compiling pass
 * */
void secondary_boot_func(void)
{
}
void internalPhyConfig(struct phy_device *phydev)
{
	/*Enable Analog and DSP register Bank access by*/
	phy_write(phydev, MDIO_DEVAD_NONE, 0x14, 0x0000);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x14, 0x0400);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x14, 0x0000);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x14, 0x0400);
	/*Write Analog register 23*/
	phy_write(phydev, MDIO_DEVAD_NONE, 0x17, 0x8E0D);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x14, 0x4417);
	/*Enable fractional PLL*/
	phy_write(phydev, MDIO_DEVAD_NONE, 0x17, 0x0005);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x14, 0x5C1B);
	//Programme fraction FR_PLL_DIV1
	phy_write(phydev, MDIO_DEVAD_NONE, 0x17, 0x029A);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x14, 0x5C1D);
	//## programme fraction FR_PLL_DiV1
	phy_write(phydev, MDIO_DEVAD_NONE, 0x17, 0xAAAA);
	phy_write(phydev, MDIO_DEVAD_NONE, 0x14, 0x5C1C);
}


static void setup_net_chip(void)
{
	eth_aml_reg0_t eth_reg0;
	*P_RESET1_LEVEL |= (1<<11);
	eth_reg0.d32 = 0;
	eth_reg0.b.phy_intf_sel = 4;
	eth_reg0.b.rx_clk_rmii_invert = 0;
	eth_reg0.b.rgmii_tx_clk_src = 0;
	eth_reg0.b.rgmii_tx_clk_phase = 0;
	eth_reg0.b.rgmii_tx_clk_ratio = 0;
	eth_reg0.b.phy_ref_clk_enable = 0;
	eth_reg0.b.clk_rmii_i_invert = 1;
	eth_reg0.b.clk_en = 1;
	eth_reg0.b.adj_enable = 0;
	eth_reg0.b.adj_setup = 0;
	eth_reg0.b.adj_delay = 0;
	eth_reg0.b.adj_skew = 0;
	eth_reg0.b.cali_start = 0;
	eth_reg0.b.cali_rise = 0;
	eth_reg0.b.cali_sel = 0;
	eth_reg0.b.rgmii_rx_reuse = 0;
	eth_reg0.b.eth_urgent = 0;
	setbits_le32(P_PREG_ETH_REG0, eth_reg0.d32);// rmii mode
	*P_PREG_ETH_REG2 = 0x10110181;
	*P_PREG_ETH_REG3 = 0xe409087f;
	setbits_le32(HHI_GCLK_MPEG1,1<<3);
	/* power on memory */
	clrbits_le32(HHI_MEM_PD_REG0, (1 << 3) | (1<<2));

}

extern struct eth_board_socket* eth_board_setup(char *name);
extern int designware_initialize(ulong base_addr, u32 interface);

int board_eth_init(bd_t *bis)
{
	setup_net_chip();
	udelay(1000);
	designware_initialize(ETH_BASE, PHY_INTERFACE_MODE_RMII);
	return 0;
}

#if CONFIG_AML_SD_EMMC
#include <mmc.h>
#include <asm/arch/sd_emmc.h>
static int  sd_emmc_init(unsigned port)
{
    switch (port)
	{
		case SDIO_PORT_A:
			break;
		case SDIO_PORT_B:
			//todo add card detect
			//setbits_le32(P_PREG_PAD_GPIO5_EN_N,1<<29);//CARD_6
			break;
		case SDIO_PORT_C:
			//enable pull up
			//clrbits_le32(P_PAD_PULL_UP_REG3, 0xff<<0);
			break;
		default:
			break;
	}

	return cpu_sd_emmc_init(port);
}

extern unsigned sd_debug_board_1bit_flag;
static int  sd_emmc_detect(unsigned port)
{
	int ret;
    switch (port) {

	case SDIO_PORT_A:
		break;
	case SDIO_PORT_B:
			setbits_le32(P_PREG_PAD_GPIO2_EN_N, 1 << 26);//CARD_6
			ret = readl(P_PREG_PAD_GPIO2_I) & (1 << 26) ? 0 : 1;
			printf("%s\n", ret ? "card in" : "card out");
			if ((readl(P_PERIPHS_PIN_MUX_6) & (3 << 8))) { //if uart pinmux set, debug board in
				if (!(readl(P_PREG_PAD_GPIO2_I) & (1 << 24))) {
					printf("sdio debug board detected, sd card with 1bit mode\n");
					sd_debug_board_1bit_flag = 1;
				}
				else{
					printf("sdio debug board detected, no sd card in\n");
					sd_debug_board_1bit_flag = 0;
					return 1;
				}
			}
		break;
	default:
		break;
	}
	return 0;
}

static void sd_emmc_pwr_prepare(unsigned port)
{
	cpu_sd_emmc_pwr_prepare(port);
}

static void sd_emmc_pwr_on(unsigned port)
{
    switch (port)
	{
		case SDIO_PORT_A:
			break;
		case SDIO_PORT_B:
//            clrbits_le32(P_PREG_PAD_GPIO5_O,(1<<31)); //CARD_8
//            clrbits_le32(P_PREG_PAD_GPIO5_EN_N,(1<<31));
			/// @todo NOT FINISH
			break;
		case SDIO_PORT_C:
			break;
		default:
			break;
	}
	return;
}
static void sd_emmc_pwr_off(unsigned port)
{
	/// @todo NOT FINISH
    switch (port)
	{
		case SDIO_PORT_A:
			break;
		case SDIO_PORT_B:
//            setbits_le32(P_PREG_PAD_GPIO5_O,(1<<31)); //CARD_8
//            clrbits_le32(P_PREG_PAD_GPIO5_EN_N,(1<<31));
			break;
		case SDIO_PORT_C:
			break;
				default:
			break;
	}
	return;
}

// #define CONFIG_TSD      1
static void board_mmc_register(unsigned port)
{
	struct aml_card_sd_info *aml_priv=cpu_sd_emmc_get(port);
    if (aml_priv == NULL)
		return;

	aml_priv->sd_emmc_init=sd_emmc_init;
	aml_priv->sd_emmc_detect=sd_emmc_detect;
	aml_priv->sd_emmc_pwr_off=sd_emmc_pwr_off;
	aml_priv->sd_emmc_pwr_on=sd_emmc_pwr_on;
	aml_priv->sd_emmc_pwr_prepare=sd_emmc_pwr_prepare;
	aml_priv->desc_buf = malloc(NEWSD_MAX_DESC_MUN*(sizeof(struct sd_emmc_desc_info)));

	if (NULL == aml_priv->desc_buf)
		printf(" desc_buf Dma alloc Fail!\n");
	else
		printf("aml_priv->desc_buf = 0x%p\n",aml_priv->desc_buf);

	sd_emmc_register(aml_priv);
}
int board_mmc_init(bd_t	*bis)
{
	__maybe_unused struct mmc *mmc;
#ifdef CONFIG_VLSI_EMULATOR
	//board_mmc_register(SDIO_PORT_A);
#else
	//board_mmc_register(SDIO_PORT_B);
#endif
	board_mmc_register(SDIO_PORT_B);
	board_mmc_register(SDIO_PORT_C);
//	board_mmc_register(SDIO_PORT_B1);

#if defined(CONFIG_ENV_IS_NOWHERE) && defined(CONFIG_AML_SD_EMMC)
	mmc = find_mmc_device(CONFIG_SYS_MMC_ENV_DEV);
	if (!mmc)
		("%s() %d: No MMC found\n", __func__, __LINE__);
	else if (mmc_init(mmc))
		printf("%s() %d: MMC init failed\n", __func__, __LINE__);
#endif
	return 0;
}

#ifdef CONFIG_SYS_I2C_AML
#if 0
static void board_i2c_set_pinmux(void){
	/*********************************************/
	/*                | I2C_Master_AO        |I2C_Slave            |       */
	/*********************************************/
	/*                | I2C_SCK                | I2C_SCK_SLAVE  |      */
	/* GPIOAO_4  | [AO_PIN_MUX: 6]     | [AO_PIN_MUX: 2]   |     */
	/*********************************************/
	/*                | I2C_SDA                 | I2C_SDA_SLAVE  |     */
	/* GPIOAO_5  | [AO_PIN_MUX: 5]     | [AO_PIN_MUX: 1]   |     */
	/*********************************************/

	//disable all other pins which share with I2C_SDA_AO & I2C_SCK_AO
	clrbits_le32(P_AO_RTI_PIN_MUX_REG, ((1<<2)|(1<<24)|(1<<1)|(1<<23)));
	//enable I2C MASTER AO pins
	setbits_le32(P_AO_RTI_PIN_MUX_REG,
	(MESON_I2C_MASTER_AO_GPIOAO_4_BIT | MESON_I2C_MASTER_AO_GPIOAO_5_BIT));

	udelay(10);
};
#endif
struct aml_i2c_platform g_aml_i2c_plat = {
	.wait_count         = 1000000,
	.wait_ack_interval  = 5,
	.wait_read_interval = 5,
	.wait_xfer_interval = 5,
	.master_no          = AML_I2C_MASTER_AO,
	.use_pio            = 0,
	.master_i2c_speed   = AML_I2C_SPPED_400K,
	.master_ao_pinmux = {
		.scl_reg    = (unsigned long)MESON_I2C_MASTER_AO_GPIOAO_4_REG,
		.scl_bit    = MESON_I2C_MASTER_AO_GPIOAO_4_BIT,
		.sda_reg    = (unsigned long)MESON_I2C_MASTER_AO_GPIOAO_5_REG,
		.sda_bit    = MESON_I2C_MASTER_AO_GPIOAO_5_BIT,
	}
};
#if 0
static void board_i2c_init(void)
{
	//set I2C pinmux with PCB board layout
	board_i2c_set_pinmux();

	//Amlogic I2C controller initialized
	//note: it must be call before any I2C operation
	aml_i2c_init();

	udelay(10);
}
#endif
#endif
#endif

#if defined(CONFIG_BOARD_EARLY_INIT_F)
int board_early_init_f(void){
	/*add board early init function here*/
	return 0;
}
#endif

#ifdef CONFIG_USB_XHCI_AMLOGIC_GXL
#include <asm/arch/usb-new.h>
#include <asm/arch/gpio.h>
#define CONFIG_GXL_USB_U2_PORT_NUM	4
#define CONFIG_GXL_USB_U3_PORT_NUM	0

static void gpio_set_vbus_power(char is_power_on)
{
	if (board_id_type_check() == HVT1_BOARD_ID_TYPE)
		return;
	if (is_power_on) {
		clrbits_le32(P_PREG_PAD_GPIO1_EN_N, (1<<24));
		setbits_le32(P_PREG_PAD_GPIO1_O, (1<<24));
	} else {
		clrbits_le32(P_PREG_PAD_GPIO1_EN_N, (1<<24));
		clrbits_le32(P_PREG_PAD_GPIO1_O, (1<<24));
	}
}

struct amlogic_usb_config g_usb_config_GXL_skt={
	CONFIG_GXL_XHCI_BASE,
	USB_ID_MODE_HARDWARE,
	gpio_set_vbus_power,
	CONFIG_GXL_USB_PHY2_BASE,
	CONFIG_GXL_USB_PHY3_BASE,
	CONFIG_GXL_USB_U2_PORT_NUM,
	CONFIG_GXL_USB_U3_PORT_NUM,
};
#endif /*CONFIG_USB_XHCI_AMLOGIC*/

#ifdef CONFIG_AML_HDMITX20
static void hdmi_tx_set_hdmi_5v(void)
{
}
#endif

extern void aml_pwm_cal_init(int mode);
extern int gpio_request(unsigned gpio, const char *label);
extern int gpio_direction_input(unsigned gpio);
extern int gpio_get_value(unsigned gpio);
extern int gpio_lookup_name(const char *name, struct udevice **devp,
		     unsigned int *offsetp, unsigned int *gpiop);
char hwid[10] = "";
void pri_board_num(void)
{
	unsigned int gpio_14,gpio_15,gpio_16,gpio_17,gpio_19;
	int val[5];

	clrbits_le32(P_PERIPHS_PIN_MUX_4,(1<<4)|(1<<5)|(1<<6)|(1<<7));//set pinmux --gpio mode
	clrbits_le32(P_PERIPHS_PIN_MUX_3,(1<<30));

	gpio_lookup_name("gpioz_14", NULL, NULL, &gpio_14);//set input mode
	gpio_lookup_name("gpioz_15", NULL, NULL, &gpio_15);
	gpio_lookup_name("gpioz_16", NULL, NULL, &gpio_16);
	gpio_lookup_name("gpioz_17", NULL, NULL, &gpio_17);
	gpio_lookup_name("gpioz_19", NULL, NULL, &gpio_19);
	gpio_request(gpio_14, "cmd_gpio");
	gpio_request(gpio_15, "cmd_gpio");
	gpio_request(gpio_16, "cmd_gpio");
	gpio_request(gpio_17, "cmd_gpio");
	gpio_request(gpio_19, "cmd_gpio");
	gpio_direction_input(gpio_14);
	gpio_direction_input(gpio_15);
	gpio_direction_input(gpio_16);
	gpio_direction_input(gpio_17);
	gpio_direction_input(gpio_19);

	val[0] = gpio_get_value(gpio_17);//get gpio status
	val[1] = gpio_get_value(gpio_16);
	val[2] = gpio_get_value(gpio_15);
	val[3] = gpio_get_value(gpio_14);
	val[4] = gpio_get_value(gpio_19);
	int i;
	char buf[10];
	for(i = 0; i < 4; ++i){
		sprintf(buf, "%d", val[i]);
		strcat(hwid, buf);
	}
	printf("SSW ID is %d\n",val[4]);
	printf("HW ID is %s\n",hwid);
}

int board_init(void)
{
#ifdef CONFIG_AML_V2_FACTORY_BURN
	//aml_try_factory_usb_burning(0, gd->bd);
#endif// #ifdef CONFIG_AML_V2_FACTORY_BURN
	aml_pwm_cal_init(0);
#ifdef CONFIG_USB_XHCI_AMLOGIC_GXL
	board_usb_init(&g_usb_config_GXL_skt,BOARD_USB_MODE_HOST);
#endif /*CONFIG_USB_XHCI_AMLOGIC*/

#ifdef CONFIG_AML_NAND
	extern int amlnf_init(unsigned char flag);
	amlnf_init(0);
#endif
    pri_board_num();
	return 0;
}


#ifdef CONFIG_BOARD_LATE_INIT
static int do_rpmb_state(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	uint32_t state = readl(AO_SEC_GP_CFG7);

	if ((state>>22) & 0x1)
		setenv("rpmb_state","1");
	else
		setenv("rpmb_state","0");

	return 0;
}

U_BOOT_CMD(rpmb_state, CONFIG_SYS_MAXARGS, 0, do_rpmb_state,
			"RPMB sub-system",
			"RPMB state\n")

int board_id_type_check(void)
{
	char buf[24] = "";
	int rtn = HVT2_BOARD_ID_TYPE;
	char buf_id[8] = "";

	if (!idme_get_var_external("board_id", buf, sizeof(buf))) {
		printf("board_id = %s\n", buf);
		if (0 == strcmp(buf, HVT1_BOARD_ID))
			rtn = HVT1_BOARD_ID_TYPE;
		else if (0 == strcmp(buf, HVT1_BOARD_ID_TYPO))
			rtn = HVT1_BOARD_ID_TYPE;
		else if ((0 == strcmp(buf, HVT2_BOARD_ID)) || (0 == strcmp(buf, HVT_BOARD_ID_B)))
			rtn = HVT2_BOARD_ID_TYPE;
		else if ((0 == strcmp(buf, EVT_BOARD_ID)) || (0 == strcmp(buf, EVT_BOARD_ID_B)))
			rtn = EVT_BOARD_ID_TYPE;
		else if ((0 == strcmp(buf, DVT_BOARD_ID)) || (0 == strcmp(buf, DVT_BOARD_ID_B)))
			rtn = DVT_BOARD_ID_TYPE;
		else if ((0 == strcmp(buf, PVT_BOARD_ID)) || (0 == strcmp(buf, PVT_BOARD_ID_B)))
			rtn = PVT_BOARD_ID_TYPE;
	}
	sprintf(buf_id, "%d", rtn);
	setenv("board_id",buf_id);

	if (HVT1_BOARD_ID_TYPE == rtn) {
		sprintf(buf_id, "%d", 1);
		setenv("bypass_standby", buf_id);
	}

	return rtn;
}

unsigned long amz_dev_flags_check(void)
{
	char buf[24] = "";
	unsigned long rtn = 0;
	char tmp_buf[8] = "";

	if (!idme_get_var_external("dev_flags", buf, sizeof(buf))){
		printf("dev_flags = %s\n", buf);
		rtn = simple_strtoul (buf, NULL, 16);
	}
	if (rtn & DEV_FLAGS_BYPASS_SECONDARY_BOOT) {
		sprintf(tmp_buf, "%d", 1);
		setenv("bypass_standby", tmp_buf);
	}

	return rtn;
}

#if defined(CONFIG_IDME)
static bool store_demo_mode(void)
{
	char usr_flags_buf[8] = {0};
	unsigned usr_flags = 0;
	bool store_demo_mode = false;

	/* treat usr_flags as an unsigned integer in hex */
	if (!idme_get_var_external("usr_flags", usr_flags_buf, sizeof(usr_flags_buf) - 1)) {
		usr_flags = simple_strtoul(usr_flags_buf, NULL, 16);
		if (usr_flags & USR_FLAGS_STOREDEMO_MODE)
			store_demo_mode = true;
	}

	printf("store demo mode: %d\n", store_demo_mode);
	return store_demo_mode;
}
#endif

/* Reset BT-module */
void reset_mt7668(void)
{
	/* Reset BT-module by reset-pin -- GPIOAO_5 */
	clrbits_le32(P_AO_GPIO_O_EN_N, 1 << 5);
	clrbits_le32(P_AO_GPIO_O_EN_N, 1 << 21);
	mdelay(200);
	setbits_le32(P_AO_GPIO_O_EN_N, 1 << 21);
	mdelay(100);
}

extern int do_setMtkBT( cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
U_BOOT_CMD(
    setMtkBT, CONFIG_SYS_MAXARGS, 1, do_setMtkBT,
    "load MTK BT driver, and set woble\n",
    NULL
);

#define PANEL_INI_PATH "/tvconfig/model/model_sum.ini"
extern int handle_panel_ini_by_name(char *panel_name, char *file_name);
int board_late_init(void)
{
	int ret;
	int led_val;
#if defined(CONFIG_IDME)
	char buf[256] = "";
#endif
	char *env;
	uint32_t reboot_mode_val = ((readl(AO_SEC_SD_CFG15) >> 12) & 0xf);
	printf("reboot_mode_val: %d\n", reboot_mode_val);

	if (is_silent_ota() && (reboot_mode_val == AMLOGIC_WATCHDOG_REBOOT ||
			reboot_mode_val == AMLOGIC_KERNEL_PANIC ||
			reboot_mode_val == AMLOGIC_CRASH_REBOOT)) {
		/* set woble before suspend. */
		do_setMtkBT(NULL, 0, 1, NULL);
		aml_system_off();
	}

	/* update rpmb state env */
	run_command("rpmb_state", 0);

	//update env before anyone using it
	run_command("get_rebootmode; echo reboot_mode=${reboot_mode}; "\
			"if test ${reboot_mode} = factory_reset; then "\
			"defenv_reserv aml_dt;setenv upgrade_step 2;save; fi;", 0);
	run_command("get_rebootmode", 0);
	env = getenv("reboot_mode");
	if(!strcmp(env,"factory_reset")){
		led_val = 2;
		send_to_led_pattern(led_val);/* send date to bl30 */
		printf("reboot_mode = %s,set 20 percent brightness,led_val=%d\n",env,led_val);
	}
	if(!strcmp(env,"update")){
		led_val = 2;
		send_to_led_pattern(led_val);/* send date to bl30 */
		printf("reboot_mode = %s,set 20 percent brightness,led_val=%d\n",env,led_val);
	}
	if(!strcmp(env,"normal")){
		led_val = 1;
		send_to_led_pattern(led_val);/* send date to bl30 */
		printf("reboot_mode = %s,breathing.led_val=%d\n",env,led_val);
	}
	if(!strcmp(env,"watchdog_reboot")){
		led_val = 1;
		send_to_led_pattern(led_val);/* send date to bl30 */
		printf("reboot_mode = %s,breathing.led_val=%d\n",env,led_val);
	}
	run_command("if itest ${upgrade_step} == 1; then "\
				"defenv_reserv; setenv upgrade_step 2; saveenv; fi;", 0);
	run_command("env default storeargs", 0);
#ifndef DTB_BIND_KERNEL
	/*add board late init function here*/
	ret = run_command("store dtb read $dtb_mem_addr", 1);
	if (ret) {
		printf("%s(): [store dtb read $dtb_mem_addr] fail\n", __func__);
		#ifdef CONFIG_DTB_MEM_ADDR
		char cmd[64];
		printf("load dtb to %x\n", CONFIG_DTB_MEM_ADDR);
		sprintf(cmd, "store dtb read %x", CONFIG_DTB_MEM_ADDR);
		ret = run_command(cmd, 1);
		if (ret) {
			printf("%s(): %s fail\n", __func__, cmd);
		}
		#endif
	}
#elif defined(CONFIG_DTB_MEM_ADDR)
	char cmd[128];
	if (!getenv("dtb_mem_addr")) {
		sprintf(cmd, "setenv dtb_mem_addr 0x%x", CONFIG_DTB_MEM_ADDR);
		run_command(cmd, 0);
	}

	sprintf(cmd, "imgread dtb boot ${dtb_mem_addr}");
	ret = run_command(cmd, 0);
	if (ret)
		printf("%s(): cmd[%s] fail, ret=%d\n", __func__, cmd, ret);
#endif// #ifndef DTB_BIND_KERNEL

	/* load unifykey */
	run_command("keyunify init 0x1234", 0);
#ifdef CONFIG_AML_VPU
	vpu_probe();
#endif
	vpp_init();
#ifdef CONFIG_AML_HDMITX20
	hdmi_tx_set_hdmi_5v();
	hdmi_tx_init();
#endif
#if defined(CONFIG_IDME)
	if (store_demo_mode())
		setenv("bypass_standby", "1");

	if (!idme_get_var_external("model_name", buf, sizeof(buf))) {
		printf("get idme model_name: %s\n", buf);
		handle_panel_ini_by_name(buf, PANEL_INI_PATH);

		if((0 == strcmp(hwid, "1111")) && (strstr(buf, "_T_") != NULL)) {
			run_command("setenv logo_fn bootup", 1);
		}else if((0 == strcmp(hwid, "1010")) && (strstr(buf, "_B_") != NULL)) {
			run_command("setenv logo_fn insigniaboot", 1);
		}else{
			run_command("setenv logo_fn amazonboot", 1);
		}
	}
	else {
		printf("get idme fail, use default model_name: %s\n", DEFAULT_MODEL_INI);
		handle_panel_ini_by_name(DEFAULT_MODEL_INI, PANEL_INI_PATH);
	}
	printf("board_id_type_check: %d\n", board_id_type_check());
	printf("amz_dev_flags_check: 0x%lu\n", amz_dev_flags_check());
#endif

#ifdef CONFIG_AML_LCD
	lcd_probe();
#if CONFIG_AMZN_SILENT_OTA
	if (!is_silent_ota()) {
		setenv("silent_ota", "0");
		setenv("bl_status", "1");
	} else {
		setenv("silent_ota", "1");
		setenv("bl_status", "0");
		printf("Found Silent OTA flag!!\n");
		clear_silent_ota_flag();
	}
#endif /* CONFIG_AMZN_SILENT_OTA */
#endif /* CONFIG_AML_LCD */

#ifdef CONFIG_AML_V2_FACTORY_BURN
	/*aml_try_factory_sdcard_burning(0, gd->bd);*/
#endif// #ifdef CONFIG_AML_V2_FACTORY_BURN

	return 0;
}
#endif

int ft_board_setup(void *blob, bd_t *bd)
{
	struct fdt_header *fdt_ptr = (struct fdt_header *)blob;
	unsigned int newsize = fdt_totalsize(fdt_ptr) + CONFIG_IDME_SIZE;

	fdt_open_into(fdt_ptr, fdt_ptr, newsize);
	idme_device_tree_initialize(fdt_ptr);
	printf("IDME inserted into FDT\n");

	return 0;
}

#ifdef CONFIG_AML_TINY_USBTOOL
int usb_get_update_result(void)
{
	unsigned long upgrade_step;
	upgrade_step = simple_strtoul (getenv ("upgrade_step"), NULL, 16);
	printf("upgrade_step = %d\n", (int)upgrade_step);
	if (upgrade_step == 1)
	{
		run_command("defenv", 1);
		run_command("setenv upgrade_step 2", 1);
		run_command("saveenv", 1);
		return 0;
	}
	else
	{
		return -1;
	}
}
#endif

phys_size_t get_effective_memsize(void)
{
	// >>16 -> MB, <<20 -> real size, so >>16<<20 = <<4
#if defined(CONFIG_SYS_MEM_TOP_HIDE)
	return (((readl(AO_SEC_GP_CFG0)) & 0xFFFF0000) << 4) - CONFIG_SYS_MEM_TOP_HIDE;
#else
	return (((readl(AO_SEC_GP_CFG0)) & 0xFFFF0000) << 4);
#endif
}

#ifdef CONFIG_MULTI_DTB
int checkhw(char * name)
{
	/*
	 * read board hw id
	 * set and select the dts according the board hw id.
	 *
	 * hwid = 1	HVT1
	 * hwid = 2	HVT2
	 */
	unsigned int hwid = HVT1_BOARD_ID_TYPE;
	char loc_name[64] = {0};

	/* read hwid */
	#ifdef CONFIG_BOARD_LATE_INIT
	hwid = board_id_type_check();
	#else
	hwid = (readl(P_AO_SEC_GP_CFG0) >> 8) & 0xFF;
	#endif

	printf("checkhw:  hwid = %d\n", hwid);


	switch (hwid) {
		case HVT1_BOARD_ID_TYPE:
			strcpy(loc_name, "blanche_cma_hvt1\0");
			break;
		default:
			strcpy(loc_name, "blanche_cma_hvt2\0");
			break;
	}
	strcpy(name, loc_name);
	setenv("aml_dt", loc_name);
	return 0;
}
#endif

static int do_logo_display(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
    char cmd_buf[512] = { 0 };

    if (argv[1] == NULL) {
        return 1;
    }

    sprintf(cmd_buf, "imgread pic logo %s $loadaddr", argv[1]);
    run_command(cmd_buf, 0);

    sprintf(cmd_buf, "bmp display $%s_offset", argv[1]);
    run_command(cmd_buf, 0);

    setenv("display_is_running", "1");
    return 0;
}

U_BOOT_CMD(
    logo_display, 3, 0, do_logo_display,
    "logo_display",
    "logo_display\n"
);

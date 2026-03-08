/*
 * board/amlogic/blanche/blanche.c
 *
 * Copyright (C) 2015 - 2020 Amlogic, Inc. All rights reserved.
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
#include <asm/arch/acs.h>
#include <asm/arch/ddr_define.h>
#include <asm/arch/timing.h>
#include <ctype.h>
#include <version.h>
#if defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_RANCHO)
#include <amzn_multiconfigs.h>
#include "fs.h"
#endif

DECLARE_GLOBAL_DATA_PTR;
//new static eth setup
struct eth_board_socket*  eth_board_skt;

int board_id_type_check(void);
int hardware_id_type_check(void);

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

#if defined UBOOT_TARGET_PRODUCT_NAME_abc123
int hardware_id_type_check(void)
{
	int var=0;
	var = simple_strtol(hwid, NULL, 2);
	printf("hardware_id_type_check: %d\n", var);
	return var;
}
#endif

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

#if defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_RANCHO)
#define IDME_BLOCK_SIZE_OLD 200
#define IDME_BLOCK_SIZE_NEW 300
#define IDME_BACKUP_OFFSET  0
#define CFG_FASTBOOT_MMC_NO (1)
#define IDME_ITEM_INIT(pitem, limit, item, max_size, export, item_permission, item_value) \
		{ \
					memset(&(pitem->data[0]), 0, max_size); \
					memcpy(&(pitem->data[0]), item_value, MIN(max_size, strlen(item_value))); \
					memset(pitem->desc.name, 0, IDME_MAX_NAME_LEN); \
					memcpy(pitem->desc.name, item, MIN(IDME_MAX_NAME_LEN, strlen(item)));\
					pitem->desc.size = max_size;\
					pitem->desc.exportable = export;\
					pitem->desc.permission = item_permission;\
				}


/* Align data in memory */
#define IDME_ITEM_NEXT(curr_item) \
	curr_item = (struct item_t *)((char *)curr_item + ((sizeof(struct idme_desc) \
	+ curr_item->desc.size + IDME_ALIGN_SIZE - 1) & (~(IDME_ALIGN_SIZE - 1))));


extern const struct idme_init_values idme_default_values[];
extern int idme_platform_write(const unsigned char *pbuf);

static int idme_platform_read_from_offset(unsigned char *pbuf, long idme_offset, int idme_block)
{
	struct mmc* mmc;
	int nread = 0;
	u64 block_offset =0;
	if (!pbuf) {
		printf("Null pbuf used in %s\n", __FUNCTION__);
		return -1;
	}

	mmc = find_mmc_device(CFG_FASTBOOT_MMC_NO);
	if (!mmc) {
		printf( "no mmc devices available\n");
		return -1;
	}
	/* switch to boot partition */
	if (mmc_switch_part(CFG_FASTBOOT_MMC_NO, CONFIG_IDME_PARTITION_NUM) != 0) {
		printf("ERROR: couldn't switch to boot partition\n");
		return -1;
	}
	/* Always use the ending part of boot partition since uboot is backed up from beginning
	 * 	capacity is usually 4M */
//	block_offset = mmc->capacity - IDME_NUM_OF_EMMC_BLOCKS * CONFIG_MMC_BLOCK_SIZE;
	if (idme_offset == -1){
	    block_offset = mmc->capacity - idme_block * CONFIG_MMC_BLOCK_SIZE;
	}else{
		block_offset = idme_offset;
	}
	printf( "%s block_offset=%lx, capacity=%lx\n",  __FUNCTION__, (long)block_offset, (long)mmc->capacity);
	nread = mmc->block_dev.block_read(CFG_FASTBOOT_MMC_NO,
	block_offset/CONFIG_MMC_BLOCK_SIZE,idme_block, pbuf);

	if (mmc_switch_part(CFG_FASTBOOT_MMC_NO, 0) != 0) {
		printf( "ERROR: couldn't switch to user partition\n");
		return -1;
	}
	if (nread < 0) {
		printf( "ERROR: idme read failure, nread %d\n", nread);
		return -1;
	}
	return 0;
}

static int idme_platform_write_offset(const unsigned char *pbuf, long idme_offset, int block_num)
{
	struct mmc* mmc;
	int nwrite = 0;
	u64 block_offset = 0;

	if (!pbuf) {
		printf( "Null pbuf used in %s\n", __FUNCTION__);
		return -1;
	}

	mmc = find_mmc_device(CFG_FASTBOOT_MMC_NO);
	if (!mmc) {
		printf( "no mmc devices available\n");
		return -1;
	}

	/* switch to boot partition */
	if (mmc_switch_part(CFG_FASTBOOT_MMC_NO, CONFIG_IDME_PARTITION_NUM) != 0) {
		printf( "ERROR: couldn't switch to boot partition\n");
		return -1;
	}
	/*
	* 	 * Always use the ending part of boot partition since uboot is backed up from beginning
	* 	 * capacity is usually 4M
	*
	 */
	if (idme_offset == -1){
		block_offset = mmc->capacity - block_num * CONFIG_MMC_BLOCK_SIZE;
	}else{
		block_offset = idme_offset;
	}

	printf( "%s block_offset=%lx, capacity=%lx\n",  __FUNCTION__, (long)block_offset, (long)mmc->capacity);
	nwrite = mmc->block_dev.block_write(CFG_FASTBOOT_MMC_NO,
						block_offset/CONFIG_MMC_BLOCK_SIZE,
						block_num, pbuf);
	if (mmc_switch_part(CFG_FASTBOOT_MMC_NO, 0) != 0) {
		printf( "ERROR: couldn't switch to user partition\n");
		return -1;
	}
	if (nwrite < 0) {
		printf( "ERROR: idme write failure, nwrite %d\n", nwrite);
		return -1;
	}
	return 0;
}

int idme_clearup()
{
	unsigned char idme_buff[CONFIG_MMC_BLOCK_SIZE * IDME_BLOCK_SIZE_OLD] __attribute__((aligned(64)));
	memset(idme_buff,0,CONFIG_MMC_BLOCK_SIZE * IDME_BLOCK_SIZE_OLD);
	if(idme_platform_write_offset(idme_buff,-1,IDME_BLOCK_SIZE_OLD) != 0){
		printf("can not clear up old idme !");
		return -1;
	}
	return 0;
}

int idme_backup(char * buff, long idme_offset_backup,int block_num)
{
	unsigned char bak[IDME_BLOCK_SIZE_OLD*CONFIG_MMC_BLOCK_SIZE] __attribute__((aligned(64)));
	struct idme_t *pidme_data = NULL;
	int iTmp;

	memset(bak, 0x00, IDME_BLOCK_SIZE_OLD*CONFIG_MMC_BLOCK_SIZE);

	if (idme_platform_read_from_offset(bak, idme_offset_backup, IDME_BLOCK_SIZE_OLD) != 0) {
		printf( "Error, failed to read idme from boot area.\n");
		return -1;
	}

	pidme_data = (struct idme_t*)&bak[0];
	iTmp = idme_check_magic_number(pidme_data);
	if (0 == iTmp) {
		printf( "Detected previous backup, leave it without touch\n");
		return 0;
	}

	if(idme_platform_write_offset(buff, idme_offset_backup, block_num) != 0){
		printf("idme backup failed\n");
		return -1;
	}
	return 0;
}

int idme_reset_items(void * new_addr, void * old_addr)
{
	struct idme_t *pidme_old = (struct idme_t *)old_addr;
	struct idme_t *pidme_new = (struct idme_t *)new_addr;
	char *idme_limit = (char *)pidme_new + CONFIG_IDME_SIZE;
	struct item_t *pitem_new = (struct item_t *)(&(pidme_new->item_data[0]));
	struct item_t *pitem_old = (struct item_t *)(&(pidme_old->item_data[0]));
	unsigned int items_num = 0;

	memset(new_addr, 0, CONFIG_IDME_SIZE);
	memcpy(pidme_new->magic, pidme_old->magic, strlen(IDME_MAGIC_NUMBER));
	memcpy(pidme_new->version, pidme_old->version, strlen(IDME_VERSION_2P1));


	/* use default values to initialize idme data */
	const struct idme_init_values *ptr_default = &idme_default_values[0];
	//const struct idme_init_values *ptr = pidme_old->item_data;
	//
	while (strlen(pitem_old->desc.name)) {

		IDME_ITEM_INIT(pitem_new, idme_limit,
				pitem_old->desc.name,
				ptr_default->desc.size,
				pitem_old->desc.exportable,
				pitem_old->desc.permission,
				pitem_old->data);
		items_num++;
	//	ptr++;
		ptr_default++;
		if (strlen(pitem_old->desc.name)){
				IDME_ITEM_NEXT(pitem_new);
				IDME_ITEM_NEXT(pitem_old);
		}
	}
	pidme_new->items_num = items_num;
	if(items_num != pidme_old->items_num){
		return -1;
	}
	return 0;
}
int idme_migrate(void)
{
	unsigned char idme_old_buff[IDME_BLOCK_SIZE_OLD*CONFIG_MMC_BLOCK_SIZE] __attribute__((aligned(64)));
	unsigned char idme_new_buff[IDME_BLOCK_SIZE_NEW*CONFIG_MMC_BLOCK_SIZE] __attribute__((aligned(64)));
	struct idme_t *pidme_data = NULL;
	int iTmp;
	u64 idme_backup_offset = IDME_BACKUP_OFFSET;
	memset(idme_old_buff, 0x00, IDME_BLOCK_SIZE_OLD*CONFIG_MMC_BLOCK_SIZE);
	memset(idme_new_buff, 0x00, IDME_BLOCK_SIZE_NEW*CONFIG_MMC_BLOCK_SIZE);

	if (idme_platform_read_from_offset(idme_old_buff, -1, IDME_BLOCK_SIZE_OLD) != 0) {
		printf( "Error, failed to read idme from boot area.\n");
 		return -1;
	}

	pidme_data = (struct idme_t*)&idme_old_buff[0];
	iTmp = idme_check_magic_number(pidme_data);
	if (-2 == iTmp) {
		printf("WARNING: Failed to find old IDME in boot area. Generating default idme ...\n");
	}else{
		printf("migrate idme from old to new !\n");
		if(idme_backup(idme_old_buff, idme_backup_offset,IDME_BLOCK_SIZE_OLD)){
			return -1;
		}
		if(idme_clearup()){
			printf("can not clear up old idme !\n");
			return -1;
		}

		if(idme_reset_items(idme_new_buff,idme_old_buff)){
			printf("error , when migrate data\n");
			return -1;
		}

		if (idme_platform_write(((const unsigned char *)idme_new_buff))){
			printf( "Error IDME: failure in idme write\n");
			return -1;
		}
	}
	return 0;
}
#endif

static bool store_demo_mode(void)
{
	char usr_flags_buf[8] = {0};
	unsigned usr_flags = 0;
	bool store_demo_mode = false;

	/* treat usr_flags as an unsigned integer in hex */
	if (!idme_get_var_external("usr_flags", usr_flags_buf, sizeof(usr_flags_buf) - 1)) {
		usr_flags = simple_strtoul(usr_flags_buf, NULL, 16);
		if (usr_flags & USR_FLAGS_STOREDEMO_MODE) {
			store_demo_mode = true;
			send_to_led_pattern(1);/* send date to bl30 */
			printf("cold boot directly,breathing\n");
		}
	}

	printf("store demo mode: %d\n", store_demo_mode);
	return store_demo_mode;
}
#endif

#if defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_RANCHO)
#define USB_STR_FILE_NAME "fac_boot_aging_exit.cvt"
static void check_usb_str(void)
{
#ifdef CONFIG_IDME
	if(idme_boot_mode() == IDME_BOOTMODE_DIAG)
	{
		mdelay(1000);
		run_command("usb start", 0);
		if(file_exists("usb", "0", USB_STR_FILE_NAME, FS_TYPE_FAT)==1)
			setenv("bypass_standby", "0");
	}
#endif
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

#define BUILD_TAG_LEN 128
#define BUILD_INFO_LEN 128
static void get_build_tag(){
	char buf[BUILD_TAG_LEN] = "unknown";
#if defined BUILD_TAG
	char * info_start = strstr(BUILD_TAG,"build");
	if(info_start && ((strlen(CONFIG_DEVICE_PRODUCT) + strlen(info_start))<(BUILD_TAG_LEN-18))){
		memset(buf,0,BUILD_TAG_LEN);
		if (strlen(BUILD_TAG) < BUILD_INFO_LEN){
			sprintf(buf, "%s_Uboot_AMZN_%s", CONFIG_DEVICE_PRODUCT,info_start);
		}
	}
#else
	if(strlen(CONFIG_DEVICE_PRODUCT)<(BUILD_TAG_LEN-24)){
		memset(buf,0,BUILD_TAG_LEN);
		sprintf(buf, "%s_Uboot_%s", CONFIG_DEVICE_PRODUCT,"localbuild");
	}
#endif

#if defined UBOOT_BUILD_TAG_SUFFIX_DIRTY
	strcat(buf,"_DIRTY");
#endif
	setenv("UbootBuildTag",buf);
}

#define PANEL_INI_PATH "/tvconfig/model/model_sum.ini"
#define PANEL_INI_NEW_PATH "/tvconfig/model/model_sum_new.ini"
#define CRI_DATA_PANEL_INI_PATH "/cri_data/PANELINI_PATH"
extern int handle_panel_ini_by_name(char *panel_name, char *file_name);
int board_late_init(void)
{
	int ret;
	int led_val;
#if defined(CONFIG_IDME)
	char buf[256] = "";
#endif
#ifdef CONFIG_CMD_WOL_POWER
	/* HERE should read idme wol power config
	DEFAULT is disable
	*/
	// SET WOL POWER
	run_command("wol_power disable", 0);
#endif
	char *env;
	uint32_t reboot_mode_val = ((readl(AO_SEC_SD_CFG15) >> 12) & 0xf);
	printf("reboot_mode_val: %d\n", reboot_mode_val);
	uint32_t val = readl(AO_GPIO_O_EN_N);
	printf("AO_GPIO_O_EN_N: %x\n", val);
#if defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_RANCHO)
	val = val & (1 << 26); // to get backlight status
#else
	val = val & (1 << 29); // to get backlight status
#endif
	if (val == 0  && (reboot_mode_val == AMLOGIC_WATCHDOG_REBOOT ||
			reboot_mode_val == AMLOGIC_KERNEL_PANIC ||
			reboot_mode_val == AMLOGIC_CRASH_REBOOT)) {
		//boot the device but keep backlight off, same as silent_ota case
		printf("Backlight is off and device triggered abnormal reboot\n");
		set_silent_ota_flag();
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
#if defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_RANCHO)
	update_tvconfig(hwid);
#endif
#ifdef CONFIG_AML_HDMITX20
	hdmi_tx_set_hdmi_5v();
	hdmi_tx_init();
#endif
#if defined(CONFIG_IDME)
	if (store_demo_mode())
		setenv("bypass_standby", "1");

	if (!idme_get_var_external("model_name", buf, sizeof(buf))) {
		printf("get idme model_name: %s\n", buf);
		if(handle_panel_ini_by_name(buf, PANEL_INI_PATH) != 0){
			printf("can not find ini file as  model_name: %s from tvconfig, try %s \n", buf, PANEL_INI_NEW_PATH);
			// Read tvconfig from PANEL_INI_NEW_PATH
			if (handle_panel_ini_by_name(buf, PANEL_INI_NEW_PATH) != 0) {
				printf("can not find ini file as  model_name: %s from tvconfig, try cri_data \n", buf);
				if(handle_panel_ini_by_name(buf, CRI_DATA_PANEL_INI_PATH) != 0){
					printf("get ini from cri_data fail, use default model_name: %s\n", DEFAULT_MODEL_INI);
					handle_panel_ini_by_name(DEFAULT_MODEL_INI, PANEL_INI_PATH);
				}
			}
		}

		if((0 == strcmp(hwid, "1111")) && (strstr(buf, "_T_") != NULL)) {
			run_command("setenv logo_fn bootup", 1);
		}else if(((0 == strcmp(hwid, "1010")) || (0 == strcmp(hwid, "1011"))) && (strstr(buf, "_B_") != NULL)) {
			run_command("setenv logo_fn insigniaboot", 1);
		}else if((0 == strcmp(hwid, "1100")) && (strstr(buf, "_B_") != NULL)) {
			run_command("setenv logo_fn insigniaboot_2", 1);
#if defined UBOOT_TARGET_PRODUCT_NAME_abc123
		}else if((0 == strcmp(hwid, "1001"))&& (strstr(buf, "_DW_") != NULL)) {
			run_command("setenv logo_fn Onidaboot", 1);
		}else if((0 == strcmp(hwid, "1001"))&& (strstr(buf, "_MM_") != NULL)) {
                        run_command("setenv logo_fn Reconnectboot", 1);
#endif
#if defined UBOOT_TARGET_PRODUCT_NAME_RANCHO
                }else if((0 == strcmp(hwid, "0111"))&& (strstr(buf, "_CROMA_") != NULL)) {
                        run_command("setenv logo_fn Croma", 1);
		}else if((0 == strcmp(hwid, "0111"))&& (strstr(buf, "_KAKAI201_") != NULL)) {
                        run_command("setenv logo_fn Akai", 1);
		}else if((0 == strcmp(hwid, "0111"))&& (strstr(buf, "_CONIDA619_") != NULL)) {
                        run_command("setenv logo_fn Onida", 1);
		}else if((0 == strcmp(hwid, "0111"))&& (strstr(buf, "_QUACHI619_") != NULL)) {
			run_command("setenv logo_fn Quachi", 1);
#endif
#if defined UBOOT_TARGET_PRODUCT_NAME_ABC
		}else if((0 == strcmp(hwid, "1110"))&& (strstr(buf, "_APB6C13_") != NULL)) {
                        run_command("setenv logo_fn Apb", 1);
		}else if((0 == strcmp(hwid, "1110"))&& (strstr(buf, "_CROMA923_") != NULL)) {
                        run_command("setenv logo_fn Croma", 1);
#endif
		}else{
			run_command("setenv logo_fn amazonboot", 1);
		}
		setenv("model_name", buf);
	}
	else {
		printf("get idme fail, use default model_name: %s\n", DEFAULT_MODEL_INI);
		handle_panel_ini_by_name(DEFAULT_MODEL_INI, PANEL_INI_PATH);
		setenv("model_name", DEFAULT_MODEL_INI);
	}
	printf("board_id_type_check: %d\n", board_id_type_check());
	printf("amz_dev_flags_check: 0x%lu\n", amz_dev_flags_check());
#endif

#if defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_RANCHO)
	check_usb_str();
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

	setenv("hwid",hwid);
	get_build_tag();
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

#if defined UBOOT_TARGET_PRODUCT_NAME_abc123
	hwid =hardware_id_type_check();
	switch (hwid) {

		case HVT1_L2_HWID_TYPE:
		case HVT1_L4_HWID_TYPE:
			strcpy(loc_name, "abc123_diwali_hvt1\0");
			break;
		case abc123_PRIME_HWID_TYPE:
		default:
			strcpy(loc_name, "abc123_prime_hvt1\0");
			break;
	}
#else
	switch (hwid) {
		case HVT1_BOARD_ID_TYPE:
			strcpy(loc_name, "blanche_cma_hvt1\0");
			break;
		default:
			strcpy(loc_name, "blanche_cma_hvt2\0");
			break;
	}
#endif
	strcpy(name, loc_name);
	setenv("aml_dt", loc_name);
	return 0;
}
#endif
#ifdef UBOOT_TARGET_PRODUCT_NAME_ABC
static int get_logo_filepath(char *logo_path, int size)
{
	int ret = -1;
	char model_name[256] = "";
#ifdef PANEL_INI_NEW_PATH
	char * filelist[] = {PANEL_INI_PATH, PANEL_INI_NEW_PATH};
#else
	char * filelist[] = {PANEL_INI_PATH};
#endif
	int i = 0;
#if defined(CONFIG_IDME)
	if (idme_get_var_external("model_name", model_name, sizeof(model_name)) != 0) {
		printf("can not get model_name ! \n");
		return ret;
	}
	printf("get idme model_name: %s\n", model_name);
#else
	#error "Must defined CONFIG_IDME"
#endif
	const char *ini_value = NULL;

	memset(logo_path, 0 , size);

	for (i = 0; i < sizeof(filelist)/sizeof(filelist[0]); i++) {
		IniParserInit();
		/**/
		printf("**Paser %s\n", filelist[i]);
		if (IniParseFile(filelist[i]) < 0) {
			printf("%s, %s load file error!\n", __func__, filelist[i]);
			IniParserUninit();
			continue;
		}

		ini_value = IniGetString(model_name, "BOOTUP_LOGO_FILE_PATH", "null");
		if (strcmp(ini_value, "null") == 0) {
			printf("%s, get \"BOOTUP_LOGO_FILE_PATH\" item failed from %s \n", __func__, filelist[i]);
			IniParserUninit();
			continue;
		} else {
			strncpy(logo_path, ini_value, size - 1);
			IniParserUninit();
			printf("%s, \"BOOTUP_LOGO_FILE_PATH\"=%s from %s \n", __func__, logo_path, filelist[i]);
			ret = 0;
			break;
		}
	}

	// check logo path.
	if (strlen(logo_path)<=0) {
		ret = -1;
		printf("%s, logo_path len is <=0, return %d\n", __func__, ret);
	}
	return ret;
}
#endif


static int do_logo_display(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
    char cmd_buf[512] = { 0 };
#ifdef UBOOT_TARGET_PRODUCT_NAME_ABC
    char logo_path[256]   = { 0 };
    int  file_size        = 0;
    unsigned int mem_addr = 0;
    int ret = -1;
#endif

    if (argv[1] == NULL) {
        return 1;
    }

#ifdef UBOOT_TARGET_PRODUCT_NAME_ABC
    if (get_logo_filepath(logo_path, sizeof(logo_path)) != 0 ) {
        printf("Can't get logo path\n");
        goto read_logo;
    }

    file_size = iniGetFileSize(logo_path);
    if (file_size <= 0) {
        printf("Can't get logo file size\n");
        goto read_logo;
    }
    char * loadaddr = getenv("loadaddr");
    if (loadaddr == NULL) {
        printf("Can't get logo memory addr\n");
        goto read_logo;
    }

    mem_addr = simple_strtoul(loadaddr, NULL, 16);
    if (iniReadFileToBuffer(logo_path, 0, file_size, mem_addr) <=0 ) {
        printf("Read logo file error\n");
        goto read_logo;
    }

    printf("Show logo from tvconfig\n");
    sprintf(cmd_buf, "bmp display $loadaddr");
    ret = run_command(cmd_buf, 0);
    if (ret) {
        printf("Show logo from tvconfig cmd:(%s) fail\n", cmd_buf);
        goto read_logo;
    }
    setenv("display_is_running", "1");
    return 0;

read_logo:
    printf("Show logo from logo partition\n");
#endif
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

#define DDR0_PUB_REG_BASE			0xff636000
#define DDR0_PUB_DCR         			( DDR0_PUB_REG_BASE + ( 0x040 << 2 ) )
#define DDR_TYPE_LPDDR2  0
#define DDR_TYPE_LPDDR3  1
#define DDR_TYPE_DDR3    3
#define DDR_TYPE_DDR4    4
static int get_ddr_info(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {

	unsigned int pub_dcr= readl(DDR0_PUB_DCR);
	printf("pub_dcr-----------0x%x-------\n",pub_dcr);
	unsigned int ddr_type = pub_dcr&0x7;
	printf(" ddr type is  %d---\n",ddr_type);
	if(ddr_type==DDR_TYPE_DDR3){
		setenv("DDR_TYPE","DDR3");
	}
	else if(ddr_type==DDR_TYPE_DDR4) {
		setenv("DDR_TYPE","DDR4");
	}
	else if(ddr_type==DDR_TYPE_LPDDR2) {
		setenv("DDR_TYPE","LPDDR2");
	}
	else if(ddr_type==DDR_TYPE_LPDDR3) {
		setenv("DDR_TYPE","LPDDR3");
	}else{
		setenv("DDR_TYPE","UNKNOWN");
		return -1;
	}
/*------------------------------------------------------method 2
	setbits_le32(P_PREG_PAD_GPIO3_O,(1<<11)|(1<<10));
	setbits_le32(P_PREG_PAD_GPIO3_EN_N,(1<<11)|(1<<10));
	unsigned int hw_subid = (readl(P_PREG_PAD_GPIO3_I) & ((1<<11)|(1<<10))) >> 10;
	printf("hw_subid %d-----\n",hw_subid);
	if (hw_subid & 1) {
		setenv("DDR_TYPE","DDR4");

	} else {
		setenv("DDR_TYPE","DDR3");
	}
*/
	return 0;
}

U_BOOT_CMD(
    memory_info, 3, 0, get_ddr_info,
    "memory_info",
    "memory_info\n"
);

#if defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_RANCHO)
static int do_led_mode(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int led_val = 3;
	char *buf = argv[1];

	if (argv[1] == NULL) {
		return 1;
	}

	if(buf[0] == '4'){
		led_val = 4;
	}else if(buf[0] == '3'){
		led_val = 3;
	}

	//led_val = atoi(argv[1]);

	send_to_led_pattern(led_val);/* send date to bl30 */
	printf("do_led_mode: led_val=%d\n",led_val);

	return 0;
}

U_BOOT_CMD(
	led_mode, 3, 0, do_led_mode,
	"led_mode",
	"led_mode\n"
);
#endif

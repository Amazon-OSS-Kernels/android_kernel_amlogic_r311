/*
 * board/amlogic/blanche/firmware/board_init.c
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

#include "power.c"
#include <asm/arch/acs.h>
#include <asm/arch/timing.h>
#include <asm/io.h>
/* bl2 customer code */
#include <asm/arch/ddr_define.h>
#define ACS_ENTRY	 0xfffc1004

#define PNL_PREG_PAD_GPIO1_EN_N           0x0f
#define PNL_PREG_PAD_GPIO1_O              0x10
#define PNL_PREG_PAD_GPIO1_I              0x11

#define PNL_PREG_PAD_GPIO3_EN_N           0x15
#define PNL_PREG_PAD_GPIO3_O              0x16
#define PNL_PREG_PAD_GPIO3_I              0x17

#define PNL_REG_BASE               (0xff634400L)
#define PNL_REG(reg)               (PNL_REG_BASE + (reg << 2))
#define PNL_REG_R(_reg)            (*(volatile unsigned int *)PNL_REG(_reg))
#define PNL_REG_W(_reg, _value)    *(volatile unsigned int *)PNL_REG(_reg) = (_value);
#define DDR3_DRV_40OHM             0
#define DDR3_ODT_60OHM             1
#define DDR3_ODT_120OHM            2
static void panel_power_init(void)
{
	serial_puts("init panel power\n");

	/* panel: GPIOZ_13/8/9 */ /* remove GPIOZ_10 for 2D/3D special case */
	PNL_REG_W(PNL_PREG_PAD_GPIO3_O,
		(PNL_REG_R(PNL_PREG_PAD_GPIO3_O) & ~((0x3 << 8) | (1 << 13))));
	PNL_REG_W(PNL_PREG_PAD_GPIO3_EN_N,
		(PNL_REG_R(PNL_PREG_PAD_GPIO3_EN_N) & ~((0x3 << 8) | (1 << 13))));
	/* panel: GPIOH_4/5 */
	PNL_REG_W(PNL_PREG_PAD_GPIO1_O,
		(PNL_REG_R(PNL_PREG_PAD_GPIO1_O) & ~(0x3 << 24)));
	PNL_REG_W(PNL_PREG_PAD_GPIO1_EN_N,
		(PNL_REG_R(PNL_PREG_PAD_GPIO1_EN_N) & ~(0x3 << 24)));

	/* backlight: GPIOZ_4/6 */
	PNL_REG_W(PNL_PREG_PAD_GPIO3_O,
		(PNL_REG_R(PNL_PREG_PAD_GPIO3_O) & ~((1 << 4) | (1 << 6))));
	PNL_REG_W(PNL_PREG_PAD_GPIO3_EN_N,
		(PNL_REG_R(PNL_PREG_PAD_GPIO3_EN_N) & ~((1 << 4) | (1 << 6))));
}

void update_ddr_parameter(void) {


#if 0


	unsigned board_id = (readl(P_AO_SEC_GP_CFG0) >> 8) & 0xFF;
	struct acs_setting *acs_entry = (struct acs_setting *)(unsigned long)readl(ACS_ENTRY);
	struct ddr_set *ddr_entry = (struct ddr_set *)(unsigned long)(acs_entry->ddr_set_addr);
#ifdef LPDDR3_REMAP_DEBUG
	serial_puts("acs_entry: 0x");
	serial_put_hex((unsigned int)(unsigned long)acs_entry, 32);
	serial_puts("\nddr_entry: 0x");
	serial_put_hex((unsigned int)(unsigned long)ddr_entry, 32);
	serial_puts("\n*ddr_entry: 0x");
	serial_put_hex(readl((unsigned long)ddr_entry), 32);
	serial_puts("\n");
#endif
	unsigned int loop=0, id_loop=0, id_loop_lpddr3=0;
	unsigned int id_total = sizeof(t_board_id_group);
	t_board_id_group *lpddr3_remap_mode = &(ddr_entry->lpddr3_remap_mode);
	for (loop=0; loop<=1; loop++, ddr_entry++) {
#ifdef LPDDR3_REMAP_DEBUG
		serial_puts("loop ");
		serial_put_dec(loop);
		serial_puts("\n");
#endif
		for (id_loop=0; id_loop<id_total; id_loop++) {
#ifdef LPDDR3_REMAP_DEBUG
			serial_puts("id_loop ");
			serial_put_dec(id_loop);
			serial_puts("\n");
#endif
			if (board_id == ddr_entry->board_id[id_loop]) {
#ifdef LPDDR3_REMAP_DEBUG
				serial_puts("board_id ");
				serial_put_dec(board_id);
#endif
				for (id_loop_lpddr3=0; id_loop_lpddr3<id_total; id_loop_lpddr3++) {
#ifdef LPDDR3_REMAP_DEBUG
					serial_puts(" id_loop_lpddr3 ");
					serial_put_dec(id_loop_lpddr3);
					serial_puts("\n");
#endif
					if (board_id == ddr_entry->board_id_lpddr3.board_id[id_loop_lpddr3]) {
						/* match lpddr3 board, config addrmap */
						update_remap(ddr_entry, lpddr3_remap_mode->board_id[id_loop_lpddr3]);
						serial_puts("Set lpddr3 remap (id:");
						serial_put_dec(board_id);
						serial_puts("-");
						serial_put_dec(lpddr3_remap_mode->board_id[id_loop_lpddr3]);
						serial_puts(")\n");
						ddr_entry->ddr_clk =  ddr_entry->lpddr3_clk;
						ddr_entry->t_pub_soc_vref_dram_vref =  ddr_entry->t_pub_soc_vref_dram_vref_lpddr3;
						ddr_entry->t_pub_acbdlr0 =  ddr_entry->t_pub_acbdlr0_lpddr3;
						ddr_entry->t_pub_acbdlr3=  ddr_entry->t_pub_acbdlr3_lpddr3;
						ddr_entry->t_pub_zq0pr =  ddr_entry->t_pub_zq0pr_lpddr3;
						ddr_entry->t_pub_zq1pr =  ddr_entry->t_pub_zq1pr_lpddr3;
						ddr_entry->t_pub_zq2pr =  ddr_entry->t_pub_zq2pr_lpddr3;
						#if 0
						ddr_entry->ddr_clk = CONFIG_LPDDR3_CLK;
						ddr_entry->t_pub_dcr = 0X89; //PUB DCR
						ddr_entry->t_pub_dtcr0 = 0x80003187; //PUB DTCR //S905 use 0x800031c7
						ddr_entry->t_pub_dtcr1 = 0x00010237;
						ddr_entry->t_pub_dsgcr = 0x02064db;
						ddr_entry->t_pub_aclcdlr = 0x0;
						ddr_entry->ddr_drv = 3;
						ddr_entry->ddr_odt = 0;
						ddr_entry->t_pub_zq0pr = 0x0ca58; //0x0ca1c,   //PUB ZQ0PR  //lpddr3
						ddr_entry->t_pub_zq1pr = 0x1cf39; //PUB ZQ1PR  38 3b
						ddr_entry->t_pub_zq2pr = 0x1cf39; //PUB ZQ2PR  38
						ddr_entry->t_pub_zq3pr = 0x1dd1d; //PUB ZQ3PR
						ddr_entry->t_pub_acbdlr0 = 0;  //CK0 delay fine tune  TAKE CARE LPDDR3 ADD/CMD DELAY
						ddr_entry->t_pub_aclcdlr = 0xf;
						ddr_entry->t_pub_acbdlr3 = 0x0; //0,  //CK0 delay fine tune b-3f  //lpddr3 tianhe 2016-10-13
						#endif
						return;
					}
				}
			}
		}
	}
	#endif
}

void ddr_pre_init(void) {

	/* ddr type pre-init */
#if 1 //(CONFIG_DDR_TYPE == CONFIG_DDR_TYPE_GPIO)
	unsigned int ddr_type = 0;
	unsigned int hw_subid = 0;

#if defined(UBOOT_TARGET_PRODUCT_NAME_ANJALI) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC)
	unsigned int hwid = 0;
#endif
	/* add your GPIO logical code here */
	/* eg:
	if (0 == GPIOXXXX)
		ddr_type = CONFIG_DDR_TYPE_DDR3;
	else
		ddr_type = CONFIG_DDR_TYPE_DDR4;
	*/
	//ddr_type = CONFIG_DDR_TYPE_DDR3;

	// txlx datasheet, pinmuxing table, page 85
	// gpioZ_10    I2S_MCLK    reg4[10]                                SDCARD_CLK  reg10[20]
	// gpioZ_11    I2S_SCLK    reg4[9]         PCM_CLK_A   reg3[6]     SDCARD_CMD  reg10[19]
	clrbits_le32(P_PERIPHS_PIN_MUX_3,1<<6);
	clrbits_le32(P_PERIPHS_PIN_MUX_4,(1<<10)|(1<<9));
	clrbits_le32(P_PERIPHS_PIN_MUX_10,(1<<20)|(1<<19));

	setbits_le32(P_PREG_PAD_GPIO3_O,(1<<11)|(1<<10));
	setbits_le32(P_PREG_PAD_GPIO3_EN_N,(1<<11)|(1<<10));
	hw_subid = (readl(P_PREG_PAD_GPIO3_I) & ((1<<11)|(1<<10))) >> 10;
	serial_puts("hw_subid = ");
	serial_put_dec(hw_subid);
	serial_puts("\n");
#if defined(UBOOT_TARGET_PRODUCT_NAME_ANJALI) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC)
	/* get hwid for change ddr frequency */
	clrbits_le32(P_PERIPHS_PIN_MUX_4,(1<<5)|(1<<6)| (1<<7));
	clrbits_le32(P_PERIPHS_PIN_MUX_3,(1<<0)|(1<<3)| (1<<24)|(1<<25)|(1<<30)|(1<<31));
	setbits_le32(P_PREG_PAD_GPIO3_O,(1<<17)|(1<<16)|(1<<15)|(1<<14));
	setbits_le32(P_PREG_PAD_GPIO3_EN_N,(1<<17)|(1<<16)|(1<<15)|(1<<14));
	hwid = (readl(P_PREG_PAD_GPIO3_I) & ((1<<17)|(1<<16)|(1<<15)|(1<<14))) >> 14;
#endif
#ifdef UBOOT_TARGET_PRODUCT_NAME_ANJALI
	if ((hwid == HVT1_L2_HWID_TYPE) ||
			(hwid == HVT1_L4_HWID_TYPE)) {
		ddr_type = CONFIG_DDR_TYPE_DDR3;
		serial_puts("ddr type DDR3\n");
	}
	else{
		ddr_type = CONFIG_DDR_TYPE_DDR4;
		serial_puts("ddr type DDR4\n");
	}
#elif defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC)
		ddr_type = CONFIG_DDR_TYPE_DDR3;
                serial_puts("ddr type DDR3\n");
#else
	if (hw_subid & 1) {
                ddr_type = CONFIG_DDR_TYPE_DDR4;
                serial_puts("ddr type DDR4\n");
        } else{
                ddr_type = CONFIG_DDR_TYPE_DDR3;
                serial_puts("ddr type DDR3\n");
        }
#endif
if(ddr_type == CONFIG_DDR_TYPE_DDR3)
{
	struct acs_setting *acs_entry = (struct acs_setting *)(unsigned long)readl(ACS_ENTRY);
	struct ddr_set *ddr_entry = (struct ddr_set *)(unsigned long)(acs_entry->ddr_set_addr);
	ddr_entry->ddr_type=ddr_type;
#ifdef UBOOT_TARGET_PRODUCT_NAME_ANJALI
	if (hwid==HVT1_L2_HWID_TYPE)	{
		serial_puts("hwid=1000\n");
		/*ddr setting for ddr3 2layer*/
		ddr_entry->ddr_clk = CONFIG_DDR_CLK_2L;
		ddr_entry->ddr_drv = DDR3_DRV_40OHM;
		ddr_entry->ddr_odt = DDR3_ODT_60OHM;
		ddr_entry->t_pub_zq0pr = 0x0007757;   /*PUB ZQ1PR//0x8fc5d, 0x4f95d,*/
		ddr_entry->t_pub_zq1pr = 0x0006fc5d;   /*PUB ZQ1PR//0x8fc5d, 0x4f95d */
		ddr_entry->t_pub_zq2pr = 0x0006fc5d;   /*PUB ZQ2PR//0x3fc5d, 0x4f95d */
		ddr_entry->t_pub_acbdlr0_1 = 0x3f;
		ddr_entry->t_pub_acbdlr3_1 = 0x18;
		ddr_entry->t_pub_aclcdlr_1 = 0x50;
	}
#elif defined(UBOOT_TARGET_PRODUCT_NAME_ABC) || defined(UBOOT_TARGET_PRODUCT_NAME_ABC)
    if (hwid == HVT1_L2_HWID_TYPE_ABC) {
        serial_puts("hwid=1110\n");
        /*ddr setting for ddr3 2layer*/
        ddr_entry->ddr_clk = CONFIG_DDR_CLK_2L;
        ddr_entry->ddr_drv = DDR3_DRV_40OHM;
        ddr_entry->ddr_odt = DDR3_ODT_120OHM;
        ddr_entry->t_pub_zq0pr = 0x59959;   /*PUB ZQ1PR//0x8fc5d, 0x4f95d,*/
        ddr_entry->t_pub_zq1pr = 0x3f95d;   /*PUB ZQ1PR//0x8fc5d, 0x4f95d */
        ddr_entry->t_pub_zq2pr = 0x3f95d;   /*PUB ZQ2PR//0x3fc5d, 0x4f95d */
        ddr_entry->t_pub_acbdlr0_1 = 0x2e;
        ddr_entry->t_pub_acbdlr3_1 = 0x0;
        ddr_entry->t_pub_aclcdlr_1 = 0x45;
    }
    else if (hwid == HVT1_L2_HWID_TYPE_ABC) {
        serial_puts("hwid=0111\n");
        /*ddr setting for ddr3 2layer*/
        ddr_entry->ddr_clk = CONFIG_DDR_CLK_2L;
        ddr_entry->ddr_drv = DDR3_DRV_40OHM;
        ddr_entry->ddr_odt = DDR3_ODT_120OHM;
        ddr_entry->t_pub_zq0pr = 0x59959;   /*PUB ZQ1PR//0x8fc5d, 0x4f95d,*/
        ddr_entry->t_pub_zq1pr = 0x3f95d;   /*PUB ZQ1PR//0x8fc5d, 0x4f95d */
        ddr_entry->t_pub_zq2pr = 0x3f95d;   /*PUB ZQ2PR//0x3fc5d, 0x4f95d */
        ddr_entry->t_pub_acbdlr0_1 = 0x2e;
        ddr_entry->t_pub_acbdlr3_1 = 0x0;
        ddr_entry->t_pub_aclcdlr_1 = 0x45;
    }
    serial_puts("ddr_pre_init pull downd GPIOZ_18 20200303\n");
    clrbits_le32(P_PREG_PAD_GPIO3_O, (1<<18));
    clrbits_le32(P_PREG_PAD_GPIO3_EN_N,(1<<18));
#endif
	ddr_entry->t_pub_acbdlr0=ddr_entry->t_pub_acbdlr0_1;
	ddr_entry->t_pub_acbdlr3=ddr_entry->t_pub_acbdlr3_1;
	ddr_entry->t_pub_aclcdlr=ddr_entry->t_pub_aclcdlr_1;
}
#endif


	update_ddr_parameter();
}
void board_init(void)
{
	power_init(0);

	panel_power_init();
	ddr_pre_init();
}

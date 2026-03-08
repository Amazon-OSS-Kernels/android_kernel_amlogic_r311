#ifndef _DI_HW_H
#define _DI_HW_H
#include "deinterlace.h"

#define	SKIP_CTRE_NUM	6
enum gate_mode_e {
	GATE_AUTO,
	GATE_ON,
	GATE_OFF,
};
void di_top_gate_control(bool top_en, bool mc_en);
void di_pre_gate_control(bool enable);
void di_post_gate_control(bool enable);
void enable_di_pre_mif(bool enable);
void enable_di_post_mif(enum gate_mode_e mode);
void di_hw_uninit(void);
void init_field_mode(unsigned short height);
void di_load_regs(struct di_pq_parm_s *di_pq_ptr);
void nr_load_regs(struct di_pq_parm_s *di_pq_ptr);
void film_mode_win_config(unsigned int width, unsigned int height);
void dump_vd2_afbc(void);
extern unsigned int DI_POST_REG_RD(unsigned int addr);
extern void di_rst_protect(bool on);
extern void di_pre_nr_wr_done_sel(bool on);
extern void di_arb_sw(bool on);

#endif

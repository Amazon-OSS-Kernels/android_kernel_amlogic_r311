#ifndef __AUDIO_INI_H__
#define __AUDIO_INI_H__

struct audio_eq_drc_info_s;

#ifdef __cplusplus
extern "C" {
#endif

int handle_audio_eq_drc_ini(char *file_name, struct audio_eq_drc_info_s *p_attr); // file_name is hdr ini file name
int handle_audio_eq_drc_ini_by_id(int proj_id, char *file_name, struct audio_eq_drc_info_s *p_attr); // file_name is summary ini file name

#ifdef __cplusplus
}
#endif

#pragma pack (1)

#define CC_REG_DATA_MAX            (128)

struct audio_reg_s {
    unsigned int len;
    unsigned int addr;
    unsigned int data[CC_REG_DATA_MAX];
};

#define CC_AUDIO_EQ_REG_CNT_MAX    (32)

struct audio_eq_drc_reg_s {
    unsigned char reg_cnt;
    struct audio_reg_s regs[CC_AUDIO_EQ_REG_CNT_MAX];
};

struct audio_eq_drc_info_s {
    unsigned int version;

    unsigned int eq_enable;
    unsigned char eq_name[32];
    unsigned int eq_byte_mode;
    struct audio_eq_drc_reg_s eq_desk;
    struct audio_eq_drc_reg_s eq_wall;

    unsigned int drc_enable;
    unsigned char drc_name[32];
    unsigned int drc_byte_mode;
    struct audio_eq_drc_reg_s drc_ead;
    struct audio_eq_drc_reg_s drc_tko;
};

#endif //__AUDIO_INI_H__

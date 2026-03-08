#ifndef __INI_PROJ_SUM_H__
#define __INI_PROJ_SUM_H__

#define CC_PANEL_INI_PATH_MASK                  (0x00000001)
#define CC_PANEL_PQ_PATH_MASK                   (0x00000002)
#define CC_HDR_PATH_MASK                        (0x00000004)
#define CC_AUD_EQ_DRC_PATH_MASK                 (0x00000008)
#define CC_AUD_CURVE_PATH_MASK                  (0x00000010)

#define CC_MAX_SUPPORT_PROJECT_CNT              (256)

struct project_info_s {
    char panel_ini_path[256];
    char panel_pq_path[256];
    char hdr_ini_path[256];
    char aud_eq_drc_ini_path[256];
    char aud_curve_ini_path[256];
};

#ifdef __cplusplus
extern "C" {
#endif

int handle_get_project_all_info(char *file_name, int *default_flag, struct project_info_s* pInfoPtr);

#ifdef __cplusplus
}
#endif

#endif //__INI_PROJ_SUM_H__

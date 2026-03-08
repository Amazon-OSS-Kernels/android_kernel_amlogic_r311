/*
 * Author:  Shoufu Zhao <shoufu.zhao@amlogic.com>
 */

#include "ini_config.h"

#define LOG_TAG "cmd_ini_test"
#define LOG_NDEBUG 0

#include "ini_log.h"

#include "ini_platform.h"
#include "model.h"
#if (defined CC_COMPILE_IN_PC || defined CC_COMPILE_IN_ANDROID)
#include "audio_ini.h"
#endif

void usage(char* processname) {
    fprintf(stderr, "Usage: %s \n", processname);
    fprintf(stderr, "\n\n");

#if (defined CC_COMPILE_IN_PC || defined CC_COMPILE_IN_ANDROID)
    fprintf(stderr, "panel command:\n");
    fprintf(stderr, "ini_test panel check\n");
    fprintf(stderr, "ini_test panel id id_num panel_summary_ini_path\n");
    fprintf(stderr, "ini_test panel xxx.ini\n");
    fprintf(stderr, "\n\n");

    fprintf(stderr, "pq command:\n");
    fprintf(stderr, "ini_test pq check --- use file path save in the panel_ini_path.\n");
    fprintf(stderr, "ini_test pq xxx.ini\n");
    fprintf(stderr, "\n\n");

    fprintf(stderr, "audio command:\n");
    fprintf(stderr, "ini_test audio eq_drc xxx.ini\n");
    fprintf(stderr, "ini_test audio eq_drc id num xxx.ini\n");
    fprintf(stderr, "\n\n");

    fprintf(stderr, "refresh:\n");
    fprintf(stderr, "ini_test refresh all panel_summary_ini_file_path.\n");
    fprintf(stderr, "ini_test refresh panel_id panel_summary_ini_file_path\n");
    fprintf(stderr, "\n\n");

    fprintf(stderr, "hdr:\n");
    fprintf(stderr, "ini_test hdr panel_id panel_summary_ini_file_path\n");
    fprintf(stderr, "\n\n");

    fprintf(stderr, "parser test:\n");
    fprintf(stderr, "ini_test parser test input_file_path output_file_path\n");
    fprintf(stderr, "\n\n");
#elif (defined CC_COMPILE_IN_UBOOT)
    printf("panel command:\n");
    printf("ini_test panel id id_num panel_summary_ini_path\n");
    printf("ini_test panel xxx.ini\n");
    printf("\n\n");
#endif

    return;
}

#if (defined CC_COMPILE_IN_PC || defined CC_COMPILE_IN_ANDROID)
int do_cmd_ini_test(int argc, char * const argv[]) {
    struct lcd_hdr_info_s hdr_attr;

#elif (defined CC_COMPILE_IN_UBOOT)
int do_cmd_ini_test(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[]) {
#endif

    if (argc < 3) {
        usage(argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "panel") == 0) {
        if (strcmp(argv[2], "id") == 0) {
            if (argc == 5) {
                handle_panel_ini_by_id(strtoul(argv[3], NULL, 0), argv[4]);
            } else {
                usage(argv[0]);
                return 0;
            }
        } else if (strcmp(argv[2], "check") == 0) { // not support in uboot
            handle_panel_ini(NULL);
        } else {
            handle_panel_ini(argv[2]);
        }
    }
#if (defined CC_COMPILE_IN_PC || defined CC_COMPILE_IN_ANDROID)
    else if (strcmp(argv[1], "audio") == 0) {
        if (strcmp(argv[2], "eq_drc") == 0) {
            if (strcmp(argv[3], "id") == 0) {
                if (argc < 6) {
                    usage(argv[0]);
                    return 0;
                }

                struct audio_eq_drc_info_s *p_attr = NULL;

                p_attr = (struct audio_eq_drc_info_s *) malloc(sizeof(struct audio_eq_drc_info_s));
                if (p_attr != NULL) {
                    handle_audio_eq_drc_ini_by_id(strtoul(argv[4], NULL, 0), argv[5], p_attr);
                    free(p_attr);
                    p_attr = NULL;
                }
            } else {
                if (argc < 4) {
                    usage(argv[0]);
                    return 0;
                }

                struct audio_eq_drc_info_s *p_attr = NULL;

                p_attr = (struct audio_eq_drc_info_s *) malloc(sizeof(struct audio_eq_drc_info_s));
                if (p_attr != NULL) {
                    handle_audio_eq_drc_ini(argv[3], p_attr);
                    free(p_attr);
                    p_attr = NULL;
                }
            }
        }
    } else if (strcmp(argv[1], "pq") == 0) {
        if (strcmp(argv[2], "check") == 0) {
            handle_panel_pq_db_path(NULL);
        } else {
            handle_panel_pq_db_path(argv[2]);
        }
    } else if (strcmp(argv[1], "refresh") == 0) {
        if (argc == 4) {
            if (strcmp(argv[2], "all") == 0) {
                handle_refresh_panel_info(1, 0, argv[3]);
            } else {
                handle_refresh_panel_info(0, strtoul(argv[2], NULL, 0), argv[3]);
            }
        } else {
            usage(argv[0]);
            return 0;
        }
    } else if (strcmp(argv[1], "hdr") == 0) {
        if (argc == 4) {
            if (handle_hdr_ini_by_id(strtoul(argv[2], NULL, 0), argv[3], &hdr_attr) < 0) {
                fprintf(stderr, "hdr info get error!!!\n\n");
                return 0;
            } else {
                fprintf(stderr, "hdr info: hdr_support = %d\n", hdr_attr.hdr_support);
                fprintf(stderr, "hdr info: hdr_features = %d\n", hdr_attr.hdr_features);
                fprintf(stderr, "hdr info: hdr_primaries_r_x = %d\n", hdr_attr.hdr_primaries_r_x);
                fprintf(stderr, "hdr info: hdr_primaries_r_y = %d\n", hdr_attr.hdr_primaries_r_y);
                fprintf(stderr, "hdr info: hdr_primaries_g_x = %d\n", hdr_attr.hdr_primaries_g_x);
                fprintf(stderr, "hdr info: hdr_primaries_g_y = %d\n", hdr_attr.hdr_primaries_g_y);
                fprintf(stderr, "hdr info: hdr_primaries_b_x = %d\n", hdr_attr.hdr_primaries_b_x);
                fprintf(stderr, "hdr info: hdr_primaries_b_y = %d\n", hdr_attr.hdr_primaries_b_y);
                fprintf(stderr, "hdr info: hdr_white_point_x = %d\n", hdr_attr.hdr_white_point_x);
                fprintf(stderr, "hdr info: hdr_white_point_y = %d\n", hdr_attr.hdr_white_point_y);
                fprintf(stderr, "hdr info: hdr_luma_max = %d\n", hdr_attr.hdr_luma_max);
                fprintf(stderr, "hdr info: hdr_luma_min = %d\n", hdr_attr.hdr_luma_min);
                fprintf(stderr, "hdr info: hdr_luma_avg = %d\n", hdr_attr.hdr_luma_avg);
                fprintf(stderr, "\n\n");
            }
        } else {
            usage(argv[0]);
            return 0;
        }
    }
#endif
    return 0;
}

#if (defined CC_COMPILE_IN_UBOOT)
U_BOOT_CMD(
   ini_test, 5, 1, do_cmd_ini_test,
   "ini test command",
   "see the useage"
);
#endif

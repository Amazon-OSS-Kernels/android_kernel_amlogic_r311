#ifndef __PAEL_INI_H__
#define __PAEL_INI_H__

#define CC_MAX_TEMP_BUF_SIZE                    (0x1000)
#define CC_MAX_PANEL_ALL_DATA_SIZE              (0x100000)

struct lcd_hdr_info_s;

#ifdef __cplusplus
extern "C" {
#endif

int handle_panel_ini(const char *file_name); // file_name is panel ini file name
int handle_panel_ini_by_id(int panel_id, char *file_name); // file_name is summary ini file name
int handle_panel_ini_by_name(char *panel_name, char *file_name);
int handle_panel_pq_db_path(const char *file_name);
int handle_refresh_panel_info(int mode, int panel_id, char *file_name);

int handle_hdr_ini(char *file_name, struct lcd_hdr_info_s *p_attr); // file_name is hdr ini file name
int handle_hdr_ini_by_id(int panel_id, char *file_name, struct lcd_hdr_info_s *hdr_attr); // file_name is summary ini file name

const char *get_def_panel_ini_name(void);
const char *get_def_panel_pq_db_name(void);

#ifdef __cplusplus
}
#endif

#pragma pack (1)

enum lcd_type_e {
    LCD_TTL = 0,
    LCD_LVDS,
    LCD_VBYONE,
    LCD_MIPI,
    LCD_EDP,
    LCD_TYPE_MAX,
};

enum bl_ctrl_method_e {
    BL_CTRL_GPIO,
    BL_CTRL_PWM,
    BL_CTRL_PWM_COMBO,
    BL_CTRL_LOCAL_DIMING,
    BL_CTRL_EXTERN,
    BL_CTRL_MAX,
};

enum bl_pwm_method_e {
    BL_PWM_NEGATIVE = 0,
    BL_PWM_POSITIVE,
    BL_PWM_METHOD_MAX,
};

enum bl_pwm_port_e {
    BL_PWM_A = 0,
    BL_PWM_B,
    BL_PWM_C,
    BL_PWM_D,
    BL_PWM_E,
    BL_PWM_F,
    BL_PWM_VS,
    BL_PWM_MAX,
};

enum lcd_extern_type_e {
    LCD_EXTERN_I2C = 0,
    LCD_EXTERN_SPI,
    LCD_EXTERN_MIPI,
    LCD_EXTERN_MAX,
};

enum lcd_extern_i2c_bus_e {
    LCD_EXTERN_I2C_BUS_AO = 0,
    LCD_EXTERN_I2C_BUS_A,
    LCD_EXTERN_I2C_BUS_B,
    LCD_EXTERN_I2C_BUS_C,
    LCD_EXTERN_I2C_BUS_D,
    LCD_EXTERN_I2C_BUS_MAX,
};

#define CC_LCD_NAME_LEN_MAX        (30)

struct lcd_header_s {
    unsigned int crc32;
    unsigned short data_len;
    unsigned short version;
    unsigned short rev;
};

struct lcd_basic_s {
    char model_name[CC_LCD_NAME_LEN_MAX];
    unsigned char lcd_type;
    unsigned char lcd_bits;
    unsigned short screen_width;  /* screen physical width in "mm" unit */
    unsigned short screen_height; /* screen physical height in "mm" unit */
};

struct lcd_timming_s {
    unsigned short h_active; /* Horizontal display area */
    unsigned short v_active; /* Vertical display area */
    unsigned short h_period; /* Horizontal total period time */
    unsigned short v_period; /* Vertical total period time */
    unsigned short hsync_width;
    unsigned short hsync_bp;
    unsigned char hsync_pol;
    unsigned short vsync_width;
    unsigned short vsync_bp;
    unsigned char vsync_pol;
};

struct lcd_customer_s {
    unsigned char fr_adjust_type;
    unsigned char ss_level;
    unsigned char clk_auto_gen;
    unsigned int pixle_clk;
    unsigned short h_period_min;
    unsigned short h_period_max;
    unsigned short v_period_min;
    unsigned short v_period_max;
    unsigned int pixle_clk_min;
    unsigned int pixle_clk_max;
    unsigned int customer_value_8;
    unsigned int customer_value_9;
};

struct lcd_interface_s {
    unsigned short if_attr_0; //vbyone_attr lane_count
    unsigned short if_attr_1; //vbyone_attr region_num
    unsigned short if_attr_2; //vbyone_attr byte_mode
    unsigned short if_attr_3; //vbyone_attr color_fmt
    unsigned short if_attr_4; //phy_attr vswing_level
    unsigned short if_attr_5; //phy_attr preemphasis_level
    unsigned short if_attr_6; //reversed
    unsigned short if_attr_7; //reversed
    unsigned short if_attr_8; //reversed
    unsigned short if_attr_9; //reversed
};

#define CC_LCD_PWR_ITEM_CNT    (4)
struct lcd_pwr_s {
    unsigned char pwr_step_type;
    unsigned char pwr_step_index;
    unsigned char pwr_step_val;
    unsigned short pwr_step_delay;
};

#define CC_MAX_PWR_SEQ_CNT     (128)

struct lcd_attr_s {
    struct lcd_header_s head;
    struct lcd_basic_s basic;
    struct lcd_timming_s timming;
    struct lcd_customer_s customer;
    struct lcd_interface_s interface;
    struct lcd_pwr_s pwr[CC_MAX_PWR_SEQ_CNT];
};

#define CC_BL_NAME_LEN_MAX        (30)

struct bl_header_s {
    unsigned int crc32;
    unsigned short data_len;
    unsigned short version;
    unsigned short rev;
};

struct bl_basic_s {
    char bl_name[CC_BL_NAME_LEN_MAX];
};

struct bl_level_s {
    unsigned short bl_level_uboot;
    unsigned short bl_level_kernel;
    unsigned short bl_level_max;
    unsigned short bl_level_min;
    unsigned short bl_level_mid;
    unsigned short bl_level_mid_mapping;
};

struct bl_method_s {
    unsigned char bl_method;
    unsigned char bl_en_gpio;
    unsigned char bl_en_gpio_on;
    unsigned char bl_en_gpio_off;
    unsigned short bl_on_delay;
    unsigned short bl_off_delay;
};

struct bl_pwm_s {
    unsigned short pwm_on_delay;
    unsigned short pwm_off_delay;

    unsigned char pwm_method;
    unsigned char pwm_port;
    unsigned int pwm_freq;
    unsigned char pwm_duty_max;
    unsigned char pwm_duty_min;
    unsigned char pwm_gpio;
    unsigned char pwm_gpio_off;

    unsigned char pwm2_method;
    unsigned char pwm2_port;
    unsigned int pwm2_freq;
    unsigned char pwm2_duty_max;
    unsigned char pwm2_duty_min;
    unsigned char pwm2_gpio;
    unsigned char pwm2_gpio_off;

    unsigned short pwm_level_max;
    unsigned short pwm_level_min;
    unsigned short pwm2_level_max;
    unsigned short pwm2_level_min;
};

struct bl_attr_s {
    struct bl_header_s head;
    struct bl_basic_s basic;
    struct bl_level_s level;
    struct bl_method_s method;
    struct bl_pwm_s pwm;
};

#define CC_LCD_EXT_NAME_LEN_MAX        (30)

struct lcd_ext_header_s {
    unsigned int crc32;
    unsigned short data_len;
    unsigned short version;
    unsigned short rev;
};

struct lcd_ext_basic_s {
    char ext_name[CC_LCD_EXT_NAME_LEN_MAX];
    unsigned char ext_index;  // set it as 0
    unsigned char ext_type;   // LCD_EXTERN_I2C, LCD_EXTERN_SPI, LCD_EXTERN_MIPI
    unsigned char ext_status; // 1 is okay, 0 is disable
};

struct lcd_ext_cmd_type_s {
    unsigned char value_0;     //i2c_addr           //spi_gpio_cs
    unsigned char value_1;     //i2c_second_addr    //spi_gpio_clk
    unsigned char value_2;     //i2c_bus            //spi_gpio_data
    unsigned char value_3;     //cmd_size           //spi_clk_freq[bit 7:0]  //unit: hz
    unsigned char value_4;                          //spi_clk_freq[bit 15:8]
    unsigned char value_5;                          //spi_clk_freq[bit 23:16]
    unsigned char value_6;                          //spi_clk_freq[bit 31:24]
    unsigned char value_7;                          //spi_clk_pol
    unsigned char value_8;                          //cmd_size
    unsigned char value_9;     //reserved for future usage
};

#define CC_EXT_CMD_MAX_CNT    (32)

struct lcd_ext_cmd_data_s {
    unsigned char init_type;
    unsigned char init_value[64];
    unsigned char init_delay;
};

struct lcd_ext_attr_s {
    struct lcd_ext_header_s head;
    struct lcd_ext_basic_s basic;
    struct lcd_ext_cmd_type_s cmd_type;
    struct lcd_ext_cmd_data_s cmd_data[CC_EXT_CMD_MAX_CNT];
};

struct panel_misc_s {
    char version[8];
    char reverse[32];
    char outputmode[64];
    char osd_reverse[12];
    char video_reverse[8];
};

#define CC_MAX_SUPPORT_PANEL_CNT          (128)

#define CC_MAX_PANEL_ALL_INFO_TAG_SIZE    (16)
#define CS_PANEL_ALL_INFO_TAG_CONTENT     "panel_all_info"

struct all_info_header_s {
    unsigned int crc32;
    unsigned short data_len;
    unsigned short version;
    unsigned char tag[CC_MAX_PANEL_ALL_INFO_TAG_SIZE];
    unsigned int max_panel_cnt;
    unsigned int cur_panel_cnt;
    unsigned int sec_off;
    unsigned int sec_cnt;
    unsigned int sec_size;
    unsigned int sec_len;
    unsigned int item_head_off;
    unsigned int item_head_cnt;
    unsigned int item_head_size;
    unsigned int item_head_len;
    unsigned int def_flag;
    unsigned char rev[12];
};

struct sec_item_s {
    unsigned short start;
    unsigned short end;
};

struct all_info_item_s {
    unsigned short off;
    unsigned short len;
};

struct all_info_item_headers_s {
    struct all_info_item_s lcd;
    struct all_info_item_s lcd_ext;
    struct all_info_item_s backlight;
    struct all_info_item_s panel_misc;
    struct all_info_item_s rev1;
    struct all_info_item_s rev2;
};

#define CC_MAX_PANEL_ALL_ONE_SEC_TAG_SIZE        (16)
#define CC_MAX_PANEL_ALL_ONE_SEC_TAG_CONTENT     "panel_all_data0"

struct all_one_sec_header_s {
    unsigned int crc32;
    unsigned short data_len;
    unsigned short version;
    unsigned char tag[CC_MAX_PANEL_ALL_ONE_SEC_TAG_SIZE];
    unsigned char rev[8];
};

struct lcd_hdr_info_s {
    unsigned int hdr_support;
    unsigned int hdr_features;
    unsigned int hdr_primaries_r_x;
    unsigned int hdr_primaries_r_y;
    unsigned int hdr_primaries_g_x;
    unsigned int hdr_primaries_g_y;
    unsigned int hdr_primaries_b_x;
    unsigned int hdr_primaries_b_y;
    unsigned int hdr_white_point_x;
    unsigned int hdr_white_point_y;
    unsigned int hdr_luma_max;
    unsigned int hdr_luma_min;
    unsigned int hdr_luma_avg;
};

#endif //__PAEL_INI_H__

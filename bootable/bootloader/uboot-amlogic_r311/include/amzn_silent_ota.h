#ifndef __AMZN_SILENT_OTA_H_
#define __AMZN_SILENT_OTA_H_

/* Silent OTA */
#define SPARSE_SPECIAL_MODE_SILENT_OTA          (1U << 13)
#define SPARSE_SCREEN_STATE                     (1U << 14)

u32 is_silent_ota(void);
void set_silent_ota_flag(void);
void clear_silent_ota_flag(void);

#endif

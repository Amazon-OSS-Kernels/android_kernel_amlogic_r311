obj-y += $(BOARD).o eth_setup.o secure_boot.o
obj-$(CONFIG_AML_LCD) += lcd.o
obj-y +=pwm_table.o

ifeq ($(CONFIG_MTK_BT_USB),y)
obj-y += ./mtk-bt/LD_btmtk_usb.o
obj-y += ./mtk-bt/LD_usbbt.o
obj-y += ./mtk-bt/cmd_btmtk.o
obj-y += ./mtk-bt/fs_platform.o
endif

ifeq ($(CONFIG_AMZN_SILENT_OTA),y)
obj-y += silent_ota.o
endif

ifeq ($(TARGET_PRODUCT_NAME_ABC), y)
obj-y += amzn_multiconfigs.o
endif

ifeq ($(TARGET_PRODUCT_NAME_RANCHO), y)
obj-y += amzn_multiconfigs.o
endif

obj-y += ini/model.o ini/ini_log.o ini/ini_core.o ini/ini_handler.o ini/ini_proxy.o ini/ini_platform.o ini/ini_io.o ini/ini_proj_sum.o ini/UnifyKey.o


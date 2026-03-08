
obj-y += $(BOARD).o eth_setup.o secure_boot.o
obj-$(CONFIG_AML_LCD) += lcd.o

obj-y += amazon_cust_config.o

ifeq ($(CONFIG_MTK_BT_USB),y)
obj-y += ./mtk-bt-76x8/LD_btmtk_usb.o
obj-y += ./mtk-bt-76x8/LD_usbbt.o
obj-y += ./mtk-bt-76x8/cmd_btmtk.o
obj-y += ./mtk-bt-76x8/fs_platform.o
endif

#
# Makefile for TVAFE.
#
obj-$(CONFIG_TVIN_AFE) += tvin_afe.o
ifdef CONFIG_AM_ATV_AUTO_SCAN
tvin_afe-objs := tvafe_adc.o tvafe_cvd_pal_m.o tvafe_general.o tvafe.o
else
tvin_afe-objs := tvafe_adc.o tvafe_cvd.o tvafe_general.o tvafe.o
endif
obj-$(CONFIG_TVIN_VBI) += tvin_vbi.o
obj-$(CONFIG_TVIN_AVIN_DETECT) += tvin_avin_detect.o
tvin_avin_detect-objs := tvafe_avin_detect.o

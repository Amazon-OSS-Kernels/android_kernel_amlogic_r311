#
# AMLOGIC VE & CM.
#

ccflags-y := -I$(PWD)/$(src)/dolby_vision/
obj-$(CONFIG_AM_VECM) += am_vecm.o
obj-$(CONFIG_AM_VDEC_DV) += amdolby_vision.o
am_vecm-objs := amve.o amcm.o amcsc.o amvecm.o keystone_correction.o bitdepth.o dnlp_cal.o vlock.o

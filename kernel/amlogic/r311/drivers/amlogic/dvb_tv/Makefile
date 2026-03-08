#
# Makefile for the DVB driver.
#

obj-$(CONFIG_AM_DVB) += aml.o

aml-objs=aml_dvb.o aml_dmx.o

obj-$(CONFIG_AM_CI) += amlci/

obj-$(CONFIG_AM_DTVDEMOD) += amldemod/

ifdef CONFIG_AM_ATV_AUTO_SCAN
obj-$(CONFIG_AM_ATVDEMOD) += amlatvdemod_auto_scan/
obj-$(CONFIG_AM_DVB) += aml_fe_auto_scan.o
else
obj-$(CONFIG_AM_ATVDEMOD) += amlatvdemod/
obj-$(CONFIG_AM_DVB) += aml_fe.o
endif

obj-$(CONFIG_AML_SMARTCARD) += smartcard/

obj-$(CONFIG_AM_SI2168) += superset/

obj-$(CONFIG_AM_SI2151) += si2151/

ccflags-y += -I.

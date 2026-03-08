#
# Makefile for the DVB driver.
#
obj-y +=amlci.o cimax.o
obj-$(CONFIG_AM_CI_CIMAX_USB) += cimax-usb.o

amlci-objs = aml_pcmcia.o aml_spi.o aml_ci.o
cimax-objs = cimax/dvb_ca_en50221_cimax.o cimax/aml_cimax.o cimax/aml_cimax_spi.o
cimax-usb-objs += cimax/usb/SRC/cimax+usb-driver.o cimax/usb/SRC/cimax+usb_fw.o
cimax-usb-objs += cimax/usb/SRC/cimax+usb_config.o
#cimax-objs += cimax/usb/SRC/cimax+usb_time.o
cimax-objs += cimax/aml_cimax_usb.o

ccflags-y += -I$(srctree)/drivers/media/dvb-core
ccflags-y += -I$(srctree)/drivers/amlogic/dvb_tv/amlci/cimax
ccflags-y += -I$(srctree)/drivers/amlogic/dvb_tv/amlci
ccflags-y += -I$(srctree)/drivers/amlogic/dvb_tv/
ccflags-y += -I$(srctree)/

ccflags-y += -I$(srctree)/drivers/amlogic/dvb_tv/amlci/cimax/usb/SRC

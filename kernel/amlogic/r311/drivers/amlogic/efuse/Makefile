#
# Makefile for eFuse.
#

ifeq ($(CONFIG_ARM64),y)
obj-$(CONFIG_EFUSE) += efuse_hw64.o efuse64.o
else ifeq ($(CONFIG_ARM64_32),y)
obj-$(CONFIG_EFUSE) += efuse_hw64.o efuse64.o
else
obj-$(CONFIG_EFUSE) += efuse_version.o efuse_hw.o efuse.o
endif

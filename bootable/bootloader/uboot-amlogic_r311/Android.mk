LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

AML_TV_UBOOT_SRC_PATH := $(LOCAL_PATH)
AML_TV_UBOOT_OUT_PATH := $(abspath $(PRODUCT_OUT)/obj/BOOTLOADER_OBJ)

AML_BL2_SRC_DIR := $(LOCAL_PATH)/../../../vendor/amlogic/common/spl

ifeq ($(AML_K32_SUPPORT), true)
CONFIG_BL_32BITS_SUPPORT=y
endif
export CONFIG_BL_32BITS_SUPPORT

ifeq ($(TARGET_PRODUCT),blanche)
AML_TV_UBOOT_BOARD := blanche
else ifeq ($(TARGET_PRODUCT),blanche_32)
AML_TV_UBOOT_BOARD := blanche
else ifeq ($(TARGET_PRODUCT),anjali)
AML_TV_UBOOT_BOARD := blanche
TARGET_PRODUCT_NAME_ANJALI=y
export TARGET_PRODUCT_NAME_ANJALI
else ifeq ($(TARGET_PRODUCT),ABC)
AML_TV_UBOOT_BOARD := blanche
TARGET_PRODUCT_NAME_ABC=y
export TARGET_PRODUCT_NAME_ABC
else ifeq ($(TARGET_PRODUCT),rancho)
AML_TV_UBOOT_BOARD := blanche
TARGET_PRODUCT_NAME_RANCHO=y
export TARGET_PRODUCT_NAME_RANCHO
else ifeq ($(TARGET_PRODUCT),abc123)
AML_TV_UBOOT_BOARD := blanche
TARGET_PRODUCT_NAME_abc123=y
export TARGET_PRODUCT_NAME_abc123
else
AML_TV_UBOOT_BOARD := txlx_t962x_r311_v1
endif

ifeq ($(wildcard $(AML_BL2_SRC_DIR)),)
BUILD_TAG_SUFFIX  := DIRTY
export BUILD_TAG_SUFFIX
$(warning "AML secure components have not been updated!")
endif

LOCAL_MODULE := aml_tv_uboot
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := build_aml_tv_uboot

.PHONY: build_aml_tv_uboot
build_aml_tv_uboot: | $(ACP)
	@mkdir -p $(PRODUCT_OUT)/unsigned/ $(AML_TV_UBOOT_OUT_PATH)
	$(MAKE) -C bootable/bootloader/uboot-amlogic_r311 \
		distclean \
		O=$(AML_TV_UBOOT_OUT_PATH)

	$(MAKE) -C bootable/bootloader/uboot-amlogic_r311 \
		KBUILD_VERBOSE=1 \
		O=$(AML_TV_UBOOT_OUT_PATH) \
			$(AML_TV_UBOOT_BOARD)_defconfig
	$(MAKE) -j 1 -C bootable/bootloader/uboot-amlogic_r311 \
		KBUILD_VERBOSE=1 \
		O=$(AML_TV_UBOOT_OUT_PATH)
	$(ACP) $(AML_TV_UBOOT_OUT_PATH)/u-boot.bin $(PRODUCT_OUT)/unsigned/bl33.bin
	# FIXME: This shouldn't be copied out from source path, we need to
	# fix the top-level Makefile eventually
	$(ACP) $(AML_TV_UBOOT_SRC_PATH)/fip/$(AML_TV_UBOOT_BOARD)/bl21.bin \
		$(AML_TV_UBOOT_SRC_PATH)/fip/$(AML_TV_UBOOT_BOARD)/acs.bin \
		$(AML_TV_UBOOT_SRC_PATH)/fip/$(AML_TV_UBOOT_BOARD)/bl301.bin \
		$(AML_TV_UBOOT_SRC_PATH)/fip/$(AML_TV_UBOOT_BOARD)/u-boot.bin \
		$(PRODUCT_OUT)/unsigned/
	$(ACP) $(AML_TV_UBOOT_SRC_PATH)/fip/$(AML_TV_UBOOT_BOARD)/u-boot.bin \
		$(PRODUCT_OUT)/unsigned/bootloader.bin
	# below line will be removed after secure SoC is ready
	$(ACP) $(AML_TV_UBOOT_SRC_PATH)/fip/$(AML_TV_UBOOT_BOARD)/u-boot.bin \
		$(PRODUCT_OUT)/bootloader.bin
	# Assume if spl dir exists, others are also exist
	$(if $(wildcard $(AML_BL2_SRC_DIR)),, \
	    $(ACP) $(AML_TV_UBOOT_SRC_PATH)/fip/$(AML_TV_UBOOT_BOARD)/bl2.bin \
		$(AML_TV_UBOOT_SRC_PATH)/fip/$(AML_TV_UBOOT_BOARD)/bl30.bin \
		$(AML_TV_UBOOT_SRC_PATH)/fip/$(AML_TV_UBOOT_BOARD)/bl31.img \
		$(AML_TV_UBOOT_SRC_PATH)/fip/$(AML_TV_UBOOT_BOARD)/bl32.img \
		$(PRODUCT_OUT)/unsigned)
	@echo "Built U-Boot successfully"

include $(BUILD_PHONY_PACKAGE)

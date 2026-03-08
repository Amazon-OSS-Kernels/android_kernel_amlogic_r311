ifneq ($(filter primrose primrosebo, $(TARGET_PRODUCT)),)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

AML_UBOOT_SRC_PATH_T962X3 := $(LOCAL_PATH)
AML_UBOOT_ROOT_PATH := $(shell dirname $(AML_UBOOT_SRC_PATH_T962X3))
AML_UBOOT_OUT_PATH_T962X3 := $(AML_UBOOT_ROOT_PATH)/bl33/build
AML_UBOOT_BIN_SOURCE_PATH_T962X3 := $(AML_UBOOT_ROOT_PATH)/build
AML_BL2_SRC_DIR_T962X3 := $(LOCAL_PATH)/../../../../../vendor/amlogictv/t962x3/spl

ifeq ($(TARGET_PRODUCT),primrose)
AML_UBOOT_BOARD_T962X3 := primrose
else ifeq ($(TARGET_PRODUCT),ABC)
AML_UBOOT_BOARD_T962X3 := primrose
TARGET_PRODUCT_NAME_ABC=y
export TARGET_PRODUCT_NAME_ABC
else ifeq ($(TARGET_PRODUCT),primrosebo)
AML_UBOOT_BOARD_T962X3 := primrose
TARGET_PRODUCT_NAME_PRIMROSEBO=y
export TARGET_PRODUCT_NAME_PRIMROSEBO
endif

ifeq ($(wildcard $(AML_BL2_SRC_DIR_T962X3)),)
BUILD_TAG_SUFFIX  := DIRTY
export BUILD_TAG_SUFFIX
$(warning "AML secure components have not been updated!")
endif

GEN_BL33 := $(AML_UBOOT_OUT_PATH_T962X3)
LOCAL_MODULE := build_aml_uboot.t962x3
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := $(GEN_BL33)

LOCAL_POST_INSTALL_CMD = $(ACP) $(AML_UBOOT_BIN_SOURCE_PATH_T962X3)/u-boot.bin $(PRODUCT_OUT)/bootloader.bin.unsigned
$(info $(LOCAL_PATH))

$(info build aml bootloader)
$(info $(AML_UBOOT_SRC_PATH_T962X3))
$(info $(AML_BL2_SRC_DIR_T962X3))

build_aml_uboot.t962x3 : $(GEN_BL33)
.PHONY: $(GEN_BL33)
$(GEN_BL33): | $(ACP)
	@mkdir -p $(PRODUCT_OUT)/unsigned/
	$(AML_UBOOT_ROOT_PATH)/bl33/AndroidBoot.mk $(AML_UBOOT_BOARD_T962X3)
	$(ACP)  $(AML_UBOOT_SRC_PATH_T962X3)/../fip/_tmp/bl33.bin  $(PRODUCT_OUT)/unsigned
	$(ACP)	$(AML_UBOOT_OUT_PATH_T962X3)/board/amlogic/$(AML_UBOOT_BOARD_T962X3)/firmware/acs.bin \
		$(AML_UBOOT_OUT_PATH_T962X3)/scp_task/bl301.bin \
		$(AML_UBOOT_OUT_PATH_T962X3)/../../fip/tm2/*.fw \
		$(PRODUCT_OUT)/unsigned

	# Assume if spl dir exists, others are also exist
	$(if $(wildcard $(AML_BL2_SRC_DIR_T962X3)),, \
	    $(ACP) $(AML_UBOOT_SRC_PATH_T962X3)/../bl2/bin/tm2/bl2.bin \
		$(AML_UBOOT_SRC_PATH_T962X3)/../bl30/bin/tm2/bl30.bin \
		$(AML_UBOOT_SRC_PATH_T962X3)/../bl31_1.3/bin/tm2/bl31.img \
		$(AML_UBOOT_SRC_PATH_T962X3)/../bl32/bin/tm2/bl32.img \
		$(PRODUCT_OUT)/unsigned)
	@echo "Built U-Boot successfully"

$(info build bootloader in $(AML_UBOOT_ROOT_PATH))
include $(BUILD_PHONY_PACKAGE)
endif #ifeq primrose

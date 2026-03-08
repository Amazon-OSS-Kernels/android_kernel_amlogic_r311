ifneq ($(filter almond ABC ABC, $(TARGET_PRODUCT)),)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

AML_UBOOT_SRC_PATH := $(LOCAL_PATH)
AML_UBOOT_ROOT_PATH := $(shell dirname $(AML_UBOOT_SRC_PATH))
AML_UBOOT_OUT_PATH := $(AML_UBOOT_ROOT_PATH)/bl33/build
AML_UBOOT_BIN_SOURCE_PATH := $(AML_UBOOT_ROOT_PATH)/build
AML_BL2_SRC_DIR := $(LOCAL_PATH)/../../../../../$(VENDOR_AML_PATH_VENDOR)/spl

ifeq ($(TARGET_PRODUCT),almond)
AML_UBOOT_BOARD_NAME := almond
UFBL_PLAT_PROJ := aml_$(VENDOR_AML_PLATFORM)
export UFBL_PLAT_PROJ

else ifeq ($(TARGET_PRODUCT),ABC)
AML_UBOOT_BOARD_NAME := ABC
UFBL_PLAT_PROJ := aml_$(VENDOR_AML_PLATFORM)
export UFBL_PLAT_PROJ

else ifeq ($(TARGET_PRODUCT),ABC)
AML_UBOOT_BOARD_NAME := ABC
UFBL_PLAT_PROJ := aml_$(VENDOR_AML_PLATFORM)
export UFBL_PLAT_PROJ

else
AML_UBOOT_BOARD_NAME := ABC
endif

ifeq ($(wildcard $(AML_BL2_SRC_DIR)),)
BUILD_TAG_SUFFIX  := DIRTY
export BUILD_TAG_SUFFIX
$(warning "AML secure components have not been updated!")
endif

GEN_BL33 := $(AML_UBOOT_OUT_PATH)
LOCAL_MODULE := build_aml_uboot.$(VENDOR_AML_PLATFORM)
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := $(GEN_BL33)

LOCAL_POST_INSTALL_CMD = $(ACP) $(AML_UBOOT_BIN_SOURCE_PATH)/u-boot.bin $(PRODUCT_OUT)/unsigned/bootloader.bin.unsigned
$(info $(LOCAL_PATH))

$(info build aml bootloader)
$(info $(AML_UBOOT_SRC_PATH))
$(info $(AML_BL2_SRC_DIR))

build_aml_uboot.$(VENDOR_AML_PLATFORM) : $(GEN_BL33)
.PHONY: $(GEN_BL33)
$(GEN_BL33): | $(ACP)
	@mkdir -p $(PRODUCT_OUT)/unsigned/
	$(AML_UBOOT_ROOT_PATH)/bl33/AndroidBoot.mk $(AML_UBOOT_BOARD_NAME) $(PLATFORM_TDK_VERSION)
	$(ACP)  $(AML_UBOOT_SRC_PATH)/../fip/_tmp/bl33.bin  $(PRODUCT_OUT)/unsigned
	$(ACP)	$(AML_UBOOT_OUT_PATH)/board/amlogic/$(AML_UBOOT_BOARD_NAME)/firmware/acs.bin \
		$(AML_UBOOT_SRC_PATH)/../bl30/src_ao/bl30.bin \
		$(PRODUCT_OUT)/unsigned
	$(ACP) $(LOCAL_PATH)/../../../../../vendor/amazon/signed-images/$(AML_UBOOT_BOARD_NAME)/user/arb_version.txt $(PRODUCT_OUT)/unsigned/
	$(ACP) $(LOCAL_PATH)/../../../../../vendor/amazon/signed-images/$(AML_UBOOT_BOARD_NAME)/ta/ta_version.txt $(PRODUCT_OUT)/unsigned/
	$(info "Built U-Boot successfully")

$(info build bootloader in $(AML_UBOOT_ROOT_PATH))
include $(BUILD_PHONY_PACKAGE)
endif

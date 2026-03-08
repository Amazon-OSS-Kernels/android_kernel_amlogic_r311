ifneq ($(filter g12a g12b, $(TARGET_BOARD_PLATFORM)),)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

OPTEE_DRV_SRC_PATH	:= $(LOCAL_PATH)
OPTEE_DRV_KMOD_PATH	:= $(abspath $(TARGET_OUT_INTERMEDIATES)/OPTEE_KMOD_OBJ)

OPTEE_DRV_SRC_FILES := \
	Makefile \
	tee_core.c \
	tee_private.h \
	tee_shm.c \
	tee_shm_pool.c \
	optee/call.c \
	optee/core.c \
	optee/Makefile \
	optee/optee_msg.h \
	optee/optee_private.h \
	optee/optee_smc.h \
	optee/rpc.c \
	optee/smccc-call.S \
	optee/supp.c \
	include/linux/arm-smccc.h \
	include/linux/tee.h \
	include/linux/tee_drv.h

# Copy all source files to $(OPTEE_DRV_KMOD_PATH)
$(OPTEE_DRV_KMOD_PATH)/%: $(OPTEE_DRV_SRC_PATH)/%
	@mkdir -p $(@D)
	@cp $< $@

copy_src: $(addprefix $(OPTEE_DRV_KMOD_PATH)/,$(OPTEE_DRV_SRC_FILES))

build_optee_drv: $(INSTALLED_KERNEL_TARGET) copy_src | $(ACP)
	$(MAKE) -C $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ M=$(OPTEE_DRV_KMOD_PATH) ARCH=arm64 \
		CROSS_COMPILE=aarch64-linux-gnu- modules
	$(ACP) `find $(OPTEE_DRV_KMOD_PATH) -name "*.ko"` $(OPTEE_DRV_SRC_PATH)
	@echo "Built OPTEE KMOD successfully"

include $(CLEAR_VARS)

LOCAL_MODULE := optee.ko
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := build_optee_drv
LOCAL_PREBUILT_MODULE_FILE := $(LOCAL_PATH)/$(LOCAL_MODULE)
$(LOCAL_PREBUILT_MODULE_FILE): build_optee_drv
ifneq ($(filter blanche,$(TARGET_DEVICE)),)
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
else
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)/boot
endif

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE := optee_armtz.ko
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
LOCAL_ADDITIONAL_DEPENDENCIES := build_optee_drv
LOCAL_PREBUILT_MODULE_FILE := $(LOCAL_PATH)/$(LOCAL_MODULE)
$(LOCAL_PREBUILT_MODULE_FILE): build_optee_drv
ifneq ($(filter blanche,$(TARGET_DEVICE)),)
LOCAL_MODULE_PATH := $(TARGET_OUT)/lib
else
LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT)/boot
endif

include $(BUILD_PREBUILT)
endif

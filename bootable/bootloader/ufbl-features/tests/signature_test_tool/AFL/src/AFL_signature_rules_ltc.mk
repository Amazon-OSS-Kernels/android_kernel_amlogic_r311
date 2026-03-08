# Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.

LOCAL_DIR := $(GET_LOCAL_DIR)
OBJS += $(LOCAL_DIR)/signature_main_afl.o
INCLUDES := -Iinclude -I$(UFBL_PATH)/include -I$(ROOTDIR)/external/libtomcrypt/src/headers
LIBRARIES:= -L../lib -ltomcrypt -ltommath -ldl
CFLAGS += -DUSE_LTM -DLTM_DESC -DLTC_SOURCE

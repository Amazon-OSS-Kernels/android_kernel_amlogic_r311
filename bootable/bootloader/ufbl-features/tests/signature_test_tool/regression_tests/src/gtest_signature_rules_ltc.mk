# Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.

LOCAL_DIR := $(GET_LOCAL_DIR)
OBJS += $(LOCAL_DIR)/test.o
LIBRARIES:= -L../lib -ltomcrypt -ltommath -ldl
INCLUDES := -Iinclude -I$(UFBL_PATH)/include -I$(ROOTDIR)/external/libtomcrypt/src/headers
CFLAGS += -DUSE_LTM -DLTM_DESC -DLTC_SOURCE
CXXFLAGS += -DLIBTOMCRYPT

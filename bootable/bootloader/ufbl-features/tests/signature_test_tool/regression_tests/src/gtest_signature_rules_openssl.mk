# Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.

LOCAL_DIR := $(GET_LOCAL_DIR)
OBJS += $(LOCAL_DIR)/test.o
LIBRARIES:= -L../lib -L$(ROOTDIR)/external/openssl -lssl -lcrypto -ldl
INCLUDES := -I$(LOCAL_PATH)/include -I$(ROOTDIR)/external/openssl/crypto/x509v3 -I$(ROOTDIR)/external/openssl/crypto/x509 -I$(UFBL_PATH)/include
CXXFLAGS += -DOPENSSL

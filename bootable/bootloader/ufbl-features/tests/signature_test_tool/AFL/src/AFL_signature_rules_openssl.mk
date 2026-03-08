# Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.

LOCAL_DIR := $(GET_LOCAL_DIR)
OBJS += $(LOCAL_DIR)/signature_main_afl.o
INCLUDES := -Iinclude -I$(ROOTDIR)/external/openssl/crypto/x509v3 -I$(ROOTDIR)/external/openssl/crypto/x509 -I$(UFBL_PATH)/include
LIBRARIES:= -L../lib -lssl -lcrypto -ldl

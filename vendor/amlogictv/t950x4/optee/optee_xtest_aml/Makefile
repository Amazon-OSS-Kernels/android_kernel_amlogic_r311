export CROSS_COMPILE?=/opt/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
export CROSS_COMPILE_TA=/opt/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-

export TDK_DIR?=$(CURDIR)/../../tdk_v38
export TA_DEV_KIT_DIR=$(ABS_OPTEE_OS_OUT_DIR)/export-${OPTEE_TA_TARGETS}
ifeq ($(ARCH), arm64)
export OPTEE_CLIENT_EXPORT=$(TDK_DIR)/ca_export_arm64
export COMPILE_NS_USER=64
else
export OPTEE_CLIENT_EXPORT=$(TDK_DIR)/ca_export_arm
export COMPILE_NS_USER=32
endif

ifeq ($O,)
out-dir := $(CURDIR)/out
else
include scripts/common.mk
out-dir := $(call strip-trailing-slashes-and-dots,$(O))
ifeq ($(out-dir),)
$(error invalid output directory (O=$(O)))
endif
endif

-include $(TA_DEV_KIT_DIR)/host_include/conf.mk
-include $(OPTEE_CLIENT_EXPORT)/include/optee_client_config.mk

ifneq ($V,1)
	q := @
	echo := @echo
else
	q :=
	echo := @:
endif
# export 'q', used by sub-makefiles.
export q

# If _HOST or _TA specific compilers are not specified, then use CROSS_COMPILE
CROSS_COMPILE_HOST ?= $(CROSS_COMPILE)
CROSS_COMPILE_TA ?= $(CROSS_COMPILE)

.PHONY: all
ifneq ($(wildcard $(TA_DEV_KIT_DIR)/host_include/conf.mk),)
all: xtest ta test_plugin
else
all:
	$(q)echo "TA_DEV_KIT_DIR is not correctly defined" && false
endif

.PHONY: xtest
xtest:
	$(q)$(MAKE) -C host/xtest CROSS_COMPILE="$(CROSS_COMPILE_HOST)" \
			     --no-builtin-variables \
			     O=$(out-dir) \
			     $@
	$(q)mv $(out-dir)/xtest/xtest $(out-dir)/xtest/tee_xtest

.PHONY: ta
ta:
	$(q)$(MAKE) -C ta CROSS_COMPILE="$(CROSS_COMPILE_TA)" \
			  O=$(out-dir) \
			  $@

.PHONY: test_plugin
test_plugin:
	$(q)$(MAKE) -C host/supp_plugin CROSS_COMPILE="$(CROSS_COMPILE_HOST)" \
			     O=$(out-dir)

.PHONY: clean
ifneq ($(wildcard $(TA_DEV_KIT_DIR)/host_include/conf.mk),)
clean:
	$(q)$(MAKE) -C host/xtest O=$(out-dir) $@
	$(q)$(MAKE) -C ta O=$(out-dir) $@
	$(q)$(MAKE) -C host/supp_plugin O=$(out-dir) $@
else
clean:
	$(q)echo "TA_DEV_KIT_DIR is not correctly defined"
	$(q)echo "You can remove manually $(out-dir)"
endif

.PHONY: checkpatch checkpatch-staging checkpatch-working
checkpatch: checkpatch-staging checkpatch-working

checkpatch-working:
	@./scripts/checkpatch.sh

checkpatch-staging:
	@./scripts/checkpatch.sh --cached

install:
	$(echo) '  INSTALL ${DESTDIR}/lib/teetz'
	$(q)mkdir -p ${DESTDIR}/lib/teetz
	$(q)find $(out-dir) -name \*.ta -exec cp -a {} ${DESTDIR}/lib/teetz \;
	$(echo) '  INSTALL ${DESTDIR}/bin'
	$(q)mkdir -p ${DESTDIR}/bin
	$(q)cp -a $(out-dir)/xtest/xtest ${DESTDIR}/bin
	$(echo) '  INSTALL ${DESTDIR}/$(CFG_TEE_PLUGIN_LOAD_PATH)'
	$(q)mkdir -p ${DESTDIR}/$(CFG_TEE_PLUGIN_LOAD_PATH)
	$(q)cp $(out-dir)/supp_plugin/*.plugin ${DESTDIR}/$(CFG_TEE_PLUGIN_LOAD_PATH)

.PHONY: cscope
cscope:
	$(echo) '  CSCOPE  .'
	${q}rm -f cscope.*
	${q}find $(PWD) -name "*.[ch]" -o -name "*.cpp" | grep -v /package/ > cscope.files
	${q}cscope -b -q -k
	$(q)cp -a $(out-dir)/xtest/xtest ${DESTDIR}/bin/tee_xtest

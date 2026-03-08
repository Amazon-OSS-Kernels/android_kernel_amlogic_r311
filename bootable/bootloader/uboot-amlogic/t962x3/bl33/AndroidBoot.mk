#!/bin/bash

export PATH=$TOP//prebuilts/gcc/linux-x86/aarch64/linaro-gcc-aarch64-4.8/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/bin:${PATH}
#export PATH=$TOP/prebuilts/gcc/linux-x86/arm/gcc-linaro-6.3.1-2017.02-x86_64_arm-linux-gnueabihf/bin:${PATH}
#export PATH=$TOP/prebuilts/gcc/linux-x86/arm/arm_eabi-2011.03/bin:${PATH}
#export PATH=$TOP/prebuilts/gcc/linux-x86/arm/CodeSourcery/CodeSourcery/Sourcery_G++_Lite/bin::${PATH}
export PATH=$TOP/vendor/amlogictv/t962x3/common/tools/riscv-none-gcc/7.2.0-4-20180606-1631/bin::${PATH}

BOOTLOADER_TARGET_BOARD=$1

AML_UBOOT_ROOT_PATH=$TOP/bootable/bootloader/uboot-amlogic/t962x3

#cd $AML_UBOOT_ROOT_PATH && ./mk $BOOTLOADER_TARGET_SOC
#cd $AML_UBOOT_ROOT_PATH && ./mk tl1_x301_v1
echo build ----- $BOOTLOADER_TARGET_BOARD
export BOOTLOADER_TARGET_BOARD

cd $AML_UBOOT_ROOT_PATH
#./mk $BOOTLOADER_PROJECT_TARGET
./mk $BOOTLOADER_TARGET_BOARD --systemroot

#$TOP/bootable/bootloader/uboot-amlogic/mk  $BOOTLOADER_TARGET_SOC

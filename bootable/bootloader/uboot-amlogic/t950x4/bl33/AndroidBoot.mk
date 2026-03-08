#!/bin/bash

export PATH=$TOP//prebuilts/gcc/linux-x86/aarch64/linaro-gcc-aarch64-4.8/gcc-linaro-aarch64-none-elf-4.8-2013.11_linux/bin:${PATH}
#export PATH=$TOP/prebuilts/gcc/linux-x86/arm/gcc-linaro-6.3.1-2017.02-x86_64_arm-linux-gnueabihf/bin:${PATH}
#export PATH=$TOP/prebuilts/gcc/linux-x86/arm/arm_eabi-2011.03/bin:${PATH}
#export PATH=$TOP/prebuilts/gcc/linux-x86/arm/CodeSourcery/CodeSourcery/Sourcery_G++_Lite/bin::${PATH}
export PATH=$TOP/${VENDOR_AML_PATH_VENDOR_COMMON}/tools/riscv-none-gcc/7.2.0-4-20180606-1631/bin::${PATH}

BOOTLOADER_TARGET_BOARD=$1

AML_UBOOT_ROOT_PATH=$TOP/bootable/bootloader/uboot-amlogic/${VENDOR_AML_PLATFORM}

#cd $AML_UBOOT_ROOT_PATH && ./mk $BOOTLOADER_TARGET_SOC
#cd $AML_UBOOT_ROOT_PATH && ./mk tl1_x301_v1
echo build ----- $BOOTLOADER_TARGET_BOARD
export BOOTLOADER_TARGET_BOARD

OPTEEOS_VER=$2
echo "******uboot soc core:${VENDOR_AML_PLATFORM_CORE}"
if [ "${VENDOR_AML_PLATFORM_CORE}" != "" ]; then
	TARGET_BL32_PATH=$AML_UBOOT_ROOT_PATH/bl32/bin/${VENDOR_AML_PLATFORM_CORE}
	if [ -d "$TARGET_BL32_PATH" ]; then

		if [ -L "$TARGET_BL32_PATH/bl32.img" ]; then
			echo "******rm $TARGET_BL32_PATH/bl32.img"
			rm $TARGET_BL32_PATH/bl32.img
		fi

		if [ -f "$TARGET_BL32_PATH/bl32.img" ]; then
			echo "******rm $TARGET_BL32_PATH/bl32.img"
			rm $TARGET_BL32_PATH/bl32.img
		fi

		cd $TARGET_BL32_PATH
		if [ "$OPTEEOS_VER" == "38" ]; then
			echo "******Link optee os 38 image to bl32.img"
			ln -s bl32_v38.img bl32.img
		else
			echo "******Link optee os 24 image to bl32.img"
			ln -s bl32_v24.img bl32.img
		fi
		cd -
	fi
fi

cd $AML_UBOOT_ROOT_PATH
#./mk $BOOTLOADER_PROJECT_TARGET
./mk $BOOTLOADER_TARGET_BOARD --systemroot

#$TOP/bootable/bootloader/uboot-amlogic/mk  $BOOTLOADER_TARGET_SOC

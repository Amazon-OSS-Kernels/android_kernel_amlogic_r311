#! /bin/bash

export CROSS_COMPILE=arm-linux-gnueabihf-

make ARCH=arm blanche32_debug_defconfig

make  ARCH=arm  UIMAGE_LOADADDR=0x00208000  uImage -j12 \
        || echo "Compile uImage Fail !!"

make ARCH=arm blanche32_cma.dtb || echo "Compile gxl dtb Fail !!"
make ARCH=arm blanche32_cma_hvt2.dtb blanche32.dtb blanche32_cma.dtb \
	blanche32_hvt2.dtb || echo "Compile gxl dtb Fail !!"

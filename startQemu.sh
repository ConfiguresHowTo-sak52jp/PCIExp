#!/bin/bash

./qemu/aarch64-softmmu/qemu-system-aarch64 \
    -M arm-generic-fdt-7series -machine linux=on \
    -serial /dev/null -serial mon:stdio -display none \
    -kernel rsrc/linux-xlnx/arch/arm/boot/uImage \
    -dtb rsrc/linux-xlnx/arch/arm/boot/dts/zynq-zc706.dtb \
    -sd rsrc/rootfs.img -append 'root=/dev/mmcblk0 rw rootwait'

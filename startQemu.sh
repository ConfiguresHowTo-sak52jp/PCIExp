#!/bin/bash

# ./qemu/aarch64-softmmu/qemu-system-aarch64 \
#     -M arm-generic-fdt-7series -machine linux=on \
#     -serial /dev/null -serial mon:stdio -display none \
#     -kernel rsrc/linux-xlnx/arch/arm/boot/uImage \
#     -dtb rsrc/linux-xlnx/arch/arm/boot/dts/zynq-zc706.dtb \
#     -sd rsrc/rootfs.img -append 'root=/dev/mmcblk0 rw rootwait'

qemu-6.0.0/build/qemu-system-aarch64 \
    -M virt -cpu cortex-a53 -nographic -smp 1 -kernel Image \
    -append "rootwait root=/dev/vda console=ttyAMA0" \
    -netdev user,id=eth0 -device virtio-net-device,netdev=eth0 \
    -drive file=rsrc/rootfs.ext4,if=none,format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0

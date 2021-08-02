# XilinxQemu
Zynq ZC-706のデバイスファイルを用いて、XilinxのqemuでLinuxを起動するサンプルである。

## linux-xlnx

submoduleとしてエントリーしてある。リポジトリは  
[https://github.com/Xilinx/linux-xlnx.git](https://github.com/Xilinx/linux-xlnx.git)  
である。
#### 使用するバージョン
tag：xilinx-v2019.2.01
#### コンフィグとビルド
下記の通り。
```
$ cd XilinxQemu/rsrc/linux-xlnx
$ cp -p ../.config.linux-xlnx .config
$ make oldconfig
$ make -j2 UIMAGE_LOADADDR=0x8000 uImage dtbs
```

## ルートファイルシステム
busyboxでベースを作成した後、初期化用のコンテンツを追加した。`rsrc/rootfs` をそのまま使うことが可能。qemuに読み込ませるために、ext4fsでフォーマットされたRAWファイルを作成する必要がある。手順は下記の通り。
```
$ cd XilinxQemu/rsrc
$ source setup.sh
$ dd if=/dev/zero of=rootfs.img bs=12M count=1
$ create_ext4img rootfs rootfs.img
```
上記操作により、rootfs.imgが作成される。尚、rootfs.imgのサイズは、格納するコンテンツより大きければ良く、特に制約はない。
### busyboxについて
submoduleである。リポジトリは[https://github.com/mirror/busybox.git](https://github.com/mirror/busybox.git)  

#### 使用するバージョン
tag：1_31_1
#### コンフィグとビルド
下記の通り。
```
$ cd XilinxQemu/rsrc
$ source setup.sh
$ cd busybox
$ cp -p ../.config.busybox .config
$ make oldconfig
$ make -j2
$ make install
```
上記操作で、`rsrc/busybox/_install` にルートファイルシステムの種が生成される。

## qemu

submoduleとしてエントリーしてある。リポジトリは  
[https://github.com/Xilinx/qemu.git](https://github.com/Xilinx/qemu.git)  
である。Xilinx QEMUの全般的な参考情報は、[こちら](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18842060/QEMU) から、また、Zynq-7000系 QEMUのスペックなどに関する情報は、[こちら](https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18842054/QEMU+-+Zynq-7000) から、それぞれ辿れる。
#### 使用するバージョン
コミットID：96f60bca06
#### コンフィグとビルド
下記の通り。
```
$ cd XilinxQemu/qemu
$ git submodule update --init dtc
$ ./configure --target-list="aarch64-softmmu,microblazeel-softmmu" --enable-fdt --disable-kvm --disable-xen
$ make -j2
```
#### 起動
```
$ cd XilinxQemu
$ ./startQemu
zynq> 
```

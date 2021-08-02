# PCIExp
下記のようなことを実験する。
#### 1. qemu-6.0.0のターゲットをarm64に指定し、自前のPCI仮想HWを組み込む。
#### 2. 上記qemu上でlinuxを走らせて、PCI仮想HWのドライバを動作させる。
#### 3. ドライバを駆動するユーザー空間上のサンプルアプリを作成する。

## 環境準備
### qemu
http://wiki.qemu.org/ からダウンロードした。今回使用したバージョンはqemu-6.0.0である。バージョンが異なると、仮想HWの仕様も激しく変わるみたいなので、注意すべし。
構築は簡単。下記の通りビルドする。

```
$ cd qemu-6.0.0
$ ./configure --target-list="aarch64-softmmu"
$ make
```
qemu-6.0.0/build/qemu-system-aarch64 が実行バイナリである。  
なお、ここでビルドしても、後述する仮想HWを組み込んで構成を変更した時は再度フルビルドが走ってしまうので、そこは諦めること。なるべく早いマシンを使うべし！
### linux
最新の安定板、LTS版は、https://cdn.kernel.org/ に纏められているので一覧すると良い。そのまま気に入ったものをダウンロード可能。今回は、その時点で最新の
LTSだった5.10.55を選んだ。ダウンロードからビルドの一連の流れは下記の通り（./rsrc/というサブディレクトリ下で作業）。

```
$ cd ./rsrc
$ source ./setup.sh # 環境変数 ARCH, CROSS_COMPILE等を設定
$ wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.10.55.tar.xz
$ tar xf linux-5.10.55.tar.xz
$ cd linux-5.10.55
$ make defconfig # 後で不都合が出たら、やり直すつもりで、特別な考慮はせず、デフォルトのまま。
$ make -j xx Image modules dtbs # 要らないかも知れないが、取り敢えず全部。本当に必要なのはImageだけ。
```
### rootfs
busyboxをビルドして作成する。ツリーは https://github.com/mirror/busybox.git からダウンロードする。コンフィグは殆どデフォルトで良いが、
雛形を準備してあるので、それを用いても良い。コンフィグからビルドは下記の通り。ただし、リポジトリから安定板と目されるタグをチェックアウトしているものとする。
ちなみに、今回使ったのは 1.33.1 というタグ。

```
$ cd .rsrc/ && source setup.sh ## 前項と同様。setupは一度だけやれば良いので前項からの連続技の時は、ここでは不要。
$ cd busybox
$ cp -p ../.config.busybox .config
$ make oldconfig
$ make -j2
$ make install
```
インストールまで成功すると、busybox/_install に下記のようなツリーが構成されるが、これではまだ足りない。
```
$ ll busybox/_install
drwxrwxr-x  5 charan charan 4096  8月  2 20:13 ./
drwxrwxr-x 36 charan charan 4096  8月  2 20:13 ../
drwxrwxr-x  2 charan charan 4096  8月  2 20:13 bin/
lrwxrwxrwx  1 charan charan   11  8月  2 20:13 linuxrc -> bin/busybox*
drwxrwxr-x  2 charan charan 4096  8月  2 20:13 sbin/
drwxrwxr-x  4 charan charan 4096  8月  2 20:13 usr/
```
下記の通りディレクトリを追加する。

```
$ cd busybox/_install
$ mkdir -p dev etc/init.d etc/network etc/profile.d lib mnt opt proc root sys tmp var/log var/run
```
ということで、全てを個別に記述しようとしたが、etc/が結構面倒なので端折る（ここが本題ではないので）。
ようはここで完成したものを下記のようにコピーするとrootfsの母体が出来上がる。
```
$ cd busybox
$ cp -pr _install ../rootfs
$ cd rsrc/rootfs
$ ll
drwxrwxr-x 15 charan charan 4096  8月  2 17:58 ./
drwxrwxr-x  6 charan charan 4096  8月  2 18:20 ../
drwxrwxr-x  2 charan charan 4096  8月  2 11:24 bin/
drwxr-xr-x  2 charan charan 4096  8月  2 13:10 dev/
drwxr-xr-x  5 charan charan 4096  8月  2 17:03 etc/
drwxrwxr-x  2 charan charan 4096  8月  2 12:35 lib/
lrwxrwxrwx  1 charan charan   11  8月  2 11:24 linuxrc -> bin/busybox*
drwxr-xr-x  2 charan charan 4096  8月  2 13:10 mnt/
drwxr-xr-x  2 charan charan 4096  8月  2 13:10 opt/
drwxr-xr-x  2 charan charan 4096  8月  2 13:10 proc/
drwxrwxr-x  2 charan charan 4096  8月  2 16:47 root/
drwxrwxr-x  2 charan charan 4096  8月  2 11:24 sbin/
drwxr-xr-x  2 charan charan 4096  8月  2 13:10 sys/
drwxr-xr-x  2 charan charan 4096  8月  2 13:10 tmp/
drwxrwxr-x  4 charan charan 4096  8月  2 11:24 usr/
drwxr-xr-x  4 charan charan 4096  8月  2 17:57 var/
```
最後にこのコンテンツを統合したイメージファイルを作成して準備完了。ここではrootfs.ext4という名前のイメージファイルを
作成している。
```
$ cd rsrc
$ dd if=/dev/zero of=rootfs.ext4 bs=12M count=1  ## ここでは12MBのファイルを作成しているが、用途に合わせてもっと大きくても良い
$ create_ext4img rootfs/ rootfs.ext4 ## create_ext4imgはsetup.shに定義されたシェル組み込み関数。中身の説明は省略。
```
## 起動
プロジェクトのルート PCIExp/ から startQemu.sh を実行すると、前項で作成したkernelとrootfsを使ってqemuが起動する。
startQemu.shの内容は下記の通り。
```
#!/bin/bash
qemu-6.0.0/build/qemu-system-aarch64 \ ## arm64アーキのエミュレーター
    -M virt -cpu cortex-a53 -nographic -smp 1 -kernel Image \ ## Imageはkernelのシンボリックリンク(*1)
    -append "rootwait root=/dev/vda console=ttyAMA0" \
    -netdev user,id=eth0 -device virtio-net-device,netdev=eth0 \
    -drive file=rsrc/rootfs.ext4,if=none,format=raw,id=hd0 \ ## rootfsはrsrc/rootfs.ext4
    -device virtio-blk-device,drive=hd0
    
# (*1)  Image -> rsrc/linux-5.10.55/arch/arm64/boot/Image
```
起動結果はこんな感じ。
```
$ cd PCIExp
$ ./startQemu
...
Starting syslogd: OK
Starting klogd: OK
Running sysctl: OK
Saving random seed: SKIP (read-only file system detected)
Starting network: ifup: interface lo already configured
ifup: interface eth0 already configured
OK
# pwd
/root
# 
```
## 仮想PCI HWの組み込み
**工事中**  
## ドライバーのインストール
**工事中**
## ユーザー空間のサンプルアプリ
**工事中**

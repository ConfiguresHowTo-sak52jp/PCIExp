# PCIExp
下記のようなことを実験する。
#### 1. qemu-6.0.0のターゲットをarm64に指定し、自前の仮想PCIデバイスを組み込む。
#### 2. 上記qemu上でlinuxを走らせて、仮想PCIデバイスのドライバを動作させる。
#### 3. ドライバを駆動するユーザー空間上のサンプルアプリを作成する。

## 参照
rafiliaさんのページを参照させて頂いた。  
https://qiita.com/rafilia/items/f7646d12212da2a85bd8  
https://github.com/rafilia/system_simulation  

大変参考になりました。ありがとうございます。


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

### クロスコンパイラ
ホストはUbuntu-18.04 x86_64なので、arm64コードが吐けるクロスコンパイラを用意する。`apt install g++-aarch64-linux-gnu`で、必要なものは全部入る（はず）。

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
$ make Image modules dtbs # 要らないかも知れないが、取り敢えず全部。本当に必要なのはImageだけ。
```
なお、リポジトリのツリーには上記で作成した `.config` を同梱してあるので、`make defconfig`の代わりに`make oldconfig && make prepare`を実行しても良い。
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
drwxrwxr-x 16 charan charan 4096  8月  8 08:20 ./
drwxrwxr-x  5 charan charan 4096  8月  8 08:22 ../
drwxrwxr-x  2 charan charan 4096  8月  7 20:09 bin/
drwxrwxr-x  2 charan charan 4096  8月  7 22:01 dev/
drwxrwxr-x  5 charan charan 4096  8月  7 20:09 etc/
drwxrwxr-x  2 charan charan 4096  8月  7 22:01 lib/
lrwxrwxrwx  1 charan charan   11  8月  7 20:09 linuxrc -> bin/busybox*
drwxrwxr-x  2 charan charan 4096  8月  7 22:01 mnt/
drwxrwxr-x  2 charan charan 4096  8月  7 22:01 opt/
drwxrwxr-x  2 charan charan 4096  8月  7 22:01 proc/
drwxrwxr-x  3 charan charan 4096  8月  7 21:59 root/
drwxrwxr-x  2 charan charan 4096  8月  8 08:20 run/
drwxrwxr-x  2 charan charan 4096  8月  7 20:09 sbin/
drwxrwxr-x  2 charan charan 4096  8月  7 22:01 sys/
drwxrwxr-x  2 charan charan 4096  8月  7 22:01 tmp/
drwxrwxr-x  4 charan charan 4096  8月  7 20:09 usr/
drwxrwxr-x  4 charan charan 4096  8月  7 20:09 var/
```
最後にこのコンテンツを統合したイメージファイルを作成して準備完了。ここではrootfs.ext4という名前のイメージファイルを
作成している。
```
$ cd rsrc
$ dd if=/dev/zero of=rootfs.ext4 bs=12M count=1  ## ここでは12MBのファイルを作成しているが、用途に合わせてもっと大きくても良い
$ create_ext4img rootfs/ rootfs.ext4 ## create_ext4imgはsetup.shに定義されたシェル組み込み関数。中身の説明は省略。
```
なお、リポジトリのrootfs/は、最初の起動に必要なコンテンツを全て含んでいるので、上記手段でimage fileを作成すればそのままqemuのルートとして使える。

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
## 仮想PCIデバイス
今回組み込んだファイルは下記の通り。
- qemu-6.0.0/hw/char/virt-pci.c (自分のオリジナル)
- qemu-6.0.0/hw/char/test_pci_device.c (参照したrafiliaさんの作品)
- qemu-6.0.0/include/hw/char/virt-pci.h (自分のオリジナル)
- qemu-6.0.0/include/hw/char/test-pci.h (参照したrafiliaさんの作品)

上記のうち、rafiliaさん作品は、こちらの環境(linux/qemu)に合わせて、若干の修正を加えさせて頂いた。
#### qemuの再ビルド
qemu-6.0.0/hw/char/meson.buildに下記の2行を加える。
```
softmmu_ss.add(when: 'CONFIG_PCI', if_true: files('virt-pci.c'))
softmmu_ss.add(when: 'CONFIG_PCI', if_true: files('test_pci_device.c'))
```
下記の通り再ビルド
```
$ cd qemu-6.6.6
$ make
```
#### 仮想PCIデバイスの概略仕様
- 一つのpioとmmioを持つ。有効サイズはとも256KB（申告サイズは512KB）。pioは実験のためだけに設けたが、使わない。
- レジスタ領域は全てmmioに割当。unprefetchable属性を持つ。
- 双方向DMAを有する。DMAを使うデモ機能として下記の２つの機能を用意
  - ドライバから入力バッファを取得し、DMAを使って内部バッファに取り込む。**次に内部バッファ値を2倍**して、DMAを用いて結果を出力バッファへ書き込む。一連の処理が終わったら割り込みを上げる。
  - ドライバから入力バッファを取得し、DMAを使って内部バッファに取り込む。**次に内部バッファ値を4倍**して、DMAを用いて結果を出力バッファへ書き込む。一連の処理が終わったら割り込みを上げる。
- マスク可能な3種類の割り込みを持つ
  1. 2倍演算終了割り込み
  2. 4倍演算終了割り込み
  3. 割り込みデモ（CTRLレジスタの所定ビットを立てると直ちに割り込みがあがる）

詳細はソースコードを参照のこと。

## デバイスドライバー
組み込んだドライバーのツリー構成は下記の通り。
```
## 自前
$ tree --charset hoge myprog/driver/
myprog/driver/
|-- Makefile
|-- vpci_drv.c
|-- vpci_drv.h
`-- vpci_drv_internal.h
```
ビルドは下記の通り。
```
$ cd rsrc && source setup.sh
$ cd myprog/driver
$ make
```
#### ドライバの概略仕様
- キャラクタドライバとして動作
  - メジャー番号はシステムから配布を受ける方式
  - udev使って、デバイスファイルまで作成するタイプ
- PCIデバイスのデバイスドライバとしての一般的な作法を踏襲
- 仮想PCIデバイスが搭載する双方向DMAのハンドリングあり
  - coherent/streaming、両バッファに対応。linux5.10.xが推奨する作法でのメモリマネージメントを実装。
- 3種類の割り込みをハンドリング
  - DMA操作を含むもの2種類、デバッグ用のもの1種類  

詳細はコードを参照のこと。

## ユーザー空間のサンプルアプリ
作成したサンプルアプリのツリー構成は下記の通り。
```
$ tree --charset hoge myprog/sample_app/
myprog/sample_app/
|-- Makefile
`-- app1.c
```
ビルドは下記の通り。
```
$ cd rsrc && source setup.sh
$ cd myprog/sample_app
$ make
```
#### サンプルアプリの概略仕様
下記のテストプログラムを逐次実行する（括弧内は対応するIOCTLコマンド）。
- 仮想PCIデバイスのバージョン確認(VPCI_IOC_GET_VERSION)
- デバッグ用割り込み実行確認(VPCI_IOC_DOINT)
- レジスタリード／ライト(VPCI_IOC_WRITE_REG/READ_REG)
- ソフトリセット(VPCI_IOC_RESET)
- 2倍演算実行(VPCI_IOC_KICK_MUL2)
- 4倍演算実行(VPCI_IOC_KICK_MUL4)

## 仮想PCIデバイスのqemuへの組み込み
【起動】で述べたstartQemu.shへのオプション指定で組み込むことが可能。具体的には下記の通り。
```
$ cd PCIExp
$ ./startQemu.sh -device virt-pci
...
Starting syslogd: OK
Starting klogd: OK
Running sysctl: OK
Saving random seed: SKIP (read-only file system detected)
Starting network: ifup: interface lo already configured
ifup: interface eth0 already configured
OK
# lspci
00:01.0 Class 00ff: 1234:0001 ## これが組み込まれた仮想PCIデバイス
00:00.0 Class 0600: 1b36:0008
```

## ビルドしたドライバ・アプリをqemuに持っていく
当初はホストのディレクトリをマウントしてqemuから実行することを考えていたが、どうにもqemuのネットワークがうまく繋がらなかったので、  
`ビルド⇒rsrc/rootfs/root/binへコピー⇒create_ext4img rootfs/ rootfs.ext4`  
という力業で凌いだ。慣れてくれば、シェルを3つくらい開いておけば`Ctrl-P`で済むので、それほど苦にはならない。

## 積み残したこと
#### 割り込みハンドラの実装変更
ボトムハーフをタスクレットで組んであるが、kernelソースを参照するとtaskletの使用は今後推奨されなさそうなので、そこで紹介されていた`request_threaded_irq()`というのを使ってみる。  
参照：https://www.kernel.org/doc/html/latest/core-api/genericirq.html?highlight=request_irq#c.request_threaded_irq  

#### 同じ機能を持った複数の仮想PCIデバイスのハンドリング
デバイスIDが異なる複数の兄弟デバイスをハンドリングするモデルを作成する。

export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-
export INSTALL_MOD_PATH=`pwd`/modules

#---- ext4fsフォーマットのRAWイメージを作成する ----
create_ext4img()
{
    if [ "x$1" = "x" ] || [ "x$2" = "x" ]; then
        cat <<EOF
Usage: create_ext4img rfsDir imgFile
Brief: Create imgFile with ext4fs, containing contents in rfsDir.
Args:
    rfsDir:
        Directory which contains contents to be transfered into imgFile.
    imgFile:
        Output image file. it should be exist & large enough to keep
        contents in rfsDir.
EOF
        return 1
    fi
    
    local tmpmnt="tmpmnt_${1}"
    local cdir=`pwd`
    local outimg=$2
    local srcdir=$1

    #--- outimgは存在しないといけない ---
    if [ ! -f $outimg ]; then
        echo "Error: $outimg not exist!!"
        return 1
    fi
    if [ ! -d $srcdir ]; then
        echo "Error: $srcdir not exist!!"
        return 1
    fi

    sudo bash -c "mkdir $tmpmnt && \
         mkfs.ext4 $outimg && \
         mount -o loop $outimg $tmpmnt && \
         cp -pr $srcdir/* $tmpmnt && \
         chown -R root:root $tmpmnt && \
         sync && \
         umount $tmpmnt && \
         rm -rf $tmpmnt"
    if [ $? -ne 0 ]; then
        echo "Error: Something wrong!!"
        #--- 残っちゃた暫定コンテンツの整理 ---
        sudo bash -c "umount $tmpmnt >/dev/null 2>&1; \
             rm -rf $tmpmnt >/dev/null 2>&1"
        return 1
    else
        echo "Success!!"
    fi

    return 0
}

#------ rootfs/の仕上げをする ----
fix_rootfs()
{
    if [ ! -d rootfs ]; then
        echo "./rootfsが存在しません"
        return 1
    fi

    #-- 必要なディレクトリを追加 --
    local dirs="proc sys dev"
    local cmd
    for d in $dirs; do
        cmd="sudo mkdir -p $d"
        echo "$cmd"
        eval $cmd
    done
    #-- dev/null追加 --
    cmd="sudo mknod dev/null c 1 3"
    echo "$cmd"
    eval $cmd
#     #-- init script追加 --
#     sudo cat <<EOF > rootfs/init
#  #!/bin/sh
#  mount -t proc none /proc
#  mount -t sysfs none /sys
#  /sbin/mdev -s
#  exec  /bin/sh
# EOF
#     cmd="sudo chmod +x rootfs/init"
#     echo "$cmd"
#     eval $cmd
    #-- modules/lib/がいたらコピーする --
    if [ -d modules/lib ]; then
        cmd="sudo cp -pr modules/lib rootfs/"
        echo "$cmd"
        eval $cmd
        cmd="sudo chown -R root:root rootfs/lib"
        echo "$cmd"
        eval $cmd
    fi

    return 0    
}

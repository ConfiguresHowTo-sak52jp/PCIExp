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


/**
 * デバッグ用サンプル
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "vpci_drv.h"

#define DEV_NAME "/dev/vpci0"

int main(int ac, char *av[])
{
    int fd = open(DEV_NAME, O_RDWR|O_SYNC);
    if (fd < 0) {
        perror("open() Failed!");
        return -1;
    }
    printf("open() success!:fd=%d\n", fd);

    // 各レジスタに連続した値を書き込んで読込結果が正しいことを確認
    VpciIoCtlParam p = {0};
    uint32_t wdata[2] = {0};
    uint32_t rdata = 0;
    for (int i = REG_VERSION/4; i <= REG_CDMA_LEN/4; i++) {
        // wdata[2]={addr,val}
        wdata[0] = 4*i;
        wdata[1] = 16+4*i;
        p.wdata = wdata;
        p.wdataNum = 2;
        // 書き込み
        int ret = ioctl(fd, VPCI_IOC_DBGWRITE, &p);
        if (ret) {
            printf("ioctl(VPCI_IOC_DBGWRITE) failed:ret=%d:%s\n",
                   ret, strerror(ret));
            return -1;
        }
        // 読み出し
        p.rdata = &rdata;
        p.rdataNum = 1;
        ret = ioctl(fd, VPCI_IOC_DBGREAD, &p);
        if (ret) {
            printf("ioctl(VPCI_IOC_DBGREAD) failed:ret=%d:%s\n",
                   ret, strerror(ret));
            return -1;
        }
        if (rdata != wdata[1]) {
            printf("Read/Write differ:write value=%u,read value=%u,reg=%d\n",
                   wdata[1], rdata, i*4);
        }
        else {
            printf("Read/Write Match!:value=%u,reg=%d\n",
                   wdata[1], i*4);
        }
    }
    return 0;
}

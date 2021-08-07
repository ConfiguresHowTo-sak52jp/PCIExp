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
#include <unistd.h>

#include "vpci_drv.h"

#define DEV_NAME "/dev/vpci0"
#define HW_VERSION (0x12345678)

int main(int ac, char *av[])
{
    int fd = open(DEV_NAME, O_RDWR|O_SYNC);
    if (fd < 0) {
        perror("open() Failed!");
        return -1;
    }
    printf("open() success!:fd=%d\n", fd);

    VpciIoCtlParam p = {0};
    uint32_t rdata = 0;

    // VPCI_IOC_GET_VERSIONのテスト
    p.val = &rdata;
    int ret = ioctl(fd, VPCI_IOC_GET_VERSION, &p);
    if (ret) {
        printf("ioctl(VPCI_IOC_GET_VERSION) failed:ret=%d:%s\n",
               ret, strerror(ret));
        return -1;
    }
    if (rdata != HW_VERSION) {
        printf("@@@@ VPCI_IOC_GET_VERSION fail:expect=%08x,actual=%08x @@@@\n",
               HW_VERSION, rdata);
    }
    else
        printf("@@@@ Phase1 success! @@@@\n");

#if 1
    // VPCI_IOC_DOINT
    printf("Issued DoInt\n");
    ret = ioctl(fd, VPCI_IOC_DOINT, &p);
    printf("DoInt DONE\n");
#endif // 0
    
#if 0
    // VPCI_IOC_WRITE_REG/READ_REGのテスト
    uint32_t wdata = 0;
    int errflag = 0;
    uint32_t expect[8];
    for (int i = 0; i < sizeof(expect)/4; i++)
        expect[i] = i*16+16;
    expect[0] = HW_VERSION; // REG_VERSION
    expect[1] = 0xbeefbeef; // REG_CTRL = W only
    expect[2] = 0; // REG_INT_STATUS = R only
    expect[7] = 0xbeefbeef; // REG_INT_CLEAR = W only
    for (int i = 0; i < sizeof(expect)/4; i++) {
        // REG_WRITE
        p.addr = 4*i;
        wdata = i*16+16;
        p.val = &wdata;
        ret = ioctl(fd, VPCI_IOC_WRITE_REG, &p);
        if (ret) {
            printf("ioctl(VPCI_IOC_WRITE_REG) failed:ret=%d:%s\n",
                   ret, strerror(ret));
            return -1;
        }
        // REG_READ
        p.val = &rdata;
        ret = ioctl(fd, VPCI_IOC_READ_REG, &p);
        if (ret) {
            printf("ioctl(VPCI_IOC_READ_REG) failed:ret=%d:%s\n",
                   ret, strerror(ret));
            return -1;
        }
        if (rdata != expect[i]) {
            printf("@@@ WRITE->READ compare fail:reg=%u,expect=%08x,"
                   "actual=%08x\n @@@", 4*i, expect[i], rdata);
            errflag = 1;
        }
    }
    if (!errflag)
        printf("@@@@ Phase2 success! @@@@\n");

    // VPCI_IOC_RESETのテスト
    ret = ioctl(fd, VPCI_IOC_RESET, &p);
    if (ret) {
        printf("ioctl(VPCI_IOC_WRITE_REG) failed:ret=%d:%s\n",
               ret, strerror(ret));
        return -1;
    }
    uint32_t rwRegs[] = {
        REG_INT_MASK,
        REG_DMA_SRC_ADDR,
        REG_DMA_DST_ADDR,
        REG_DMA_SIZE,
    };
    for (int i = 0; i < sizeof(rwRegs)/4; i++) {
        expect[i] = 0;
    }
    expect[0] = 0xffffffff;
    errflag = 0;
    for (int i = 0; i < sizeof(rwRegs)/4; i++) {
        p.addr = rwRegs[i];
        p.val = &rdata;
        ret = ioctl(fd, VPCI_IOC_READ_REG, &p);
        if (ret) {
            printf("ioctl(VPCI_IOC_READ_REG) failed:ret=%d:%s\n",
                   ret, strerror(ret));
            return -1;
        }
        if (rdata != expect[i]) {
            printf("@@@ RESET fail:reg=%u,expect=%08x,"
                   "actual=%08x\n @@@", rwRegs[i], expect[i], rdata);
            errflag = 1;
        }
    }
    if (!errflag)
        printf("@@@@ Phase3 success! @@@@\n");
#endif // 0
    
    // VPCI_IOC_KICK_MUL2のテスト
    uint32_t max = 0x80000000;
    uint32_t *expect2 = malloc(4096);
    for (int i = 0; i < 4096/4; i++)
        expect2[i] = rand()%max;
    expect2[0] = 0x7fffffff;
    expect2[1023] = 0;
    p.inData = expect2;
    p.outData = malloc(4096);
    p.dataNum = 4096/4;
    ret = ioctl(fd, VPCI_IOC_KICK_MUL2, &p);
    if (ret) {
        printf("ioctl(VPCI_IOC_KICK_MUL2) failed:ret=%d:%s\n",
               ret, strerror(ret));
        return -1;
    }
    int errflag = 0;
    for (int i = 0; i < 4096/4; i++) {
        if (p.outData[i] != expect2[i]*2) {
            printf("@@@ VPCI_IOC_KICK_MUL2 compare fail:expect=%08x,"
                   "actual=%08x\n @@@", 2*expect2[i], p.outData[i]);
            errflag = 1;
         }
    }
    if (!errflag)
        printf("@@@@ Phase4 success! @@@@\n");
    free(p.outData);

    // VPCI_IOC_KICK_MUL4のテスト
    max = 0x40000000;
    for (int i = 0; i < 4096/4; i++)
        expect2[i] = rand()%max;
    expect2[0] = 0x3fffffff;
    expect2[1023] = 0;
    p.inData = expect2;
    p.outData = malloc(4096);
    p.dataNum = 4096/4;
    ret = ioctl(fd, VPCI_IOC_KICK_MUL4, &p);
    if (ret) {
        printf("ioctl(VPCI_IOC_KICK_MUL4) failed:ret=%d:%s\n",
               ret, strerror(ret));
        return -1;
    }
    errflag = 0;
    for (int i = 0; i < 4096/4; i++) {
        if (p.outData[i] != expect2[i]*4) {
            printf("@@@ VPCI_IOC_KICK_MUL4 compare fail:expect=%08x,"
                   "actual=%08x\n @@@", 4*expect2[i], p.outData[i]);
            errflag = 1;
         }
    }
    if (!errflag)
        printf("@@@@ Phase5 success! @@@@\n");
    free(p.outData);
    
    close(fd);
    
    return 0;
}

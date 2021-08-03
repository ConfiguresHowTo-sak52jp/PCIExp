#ifndef _VPCI_DRV_H_
#define _VPCI_DRV_H_

#include <linux/types.h>

/*
 * 本番ではレジスタ定義をユーザー空間に公開することはないが、検討段階では
 * デバッグをやりやすくするためにレジスタのオフセットを共有する。
 */
//--- 仮想レジスタアドレス定義 ---
// Version
#define REG_VERSION         (0)
// Contol
#define REG_CTRL            (4)
// Interrupt status
#define REG_INT_STATUS      (8)
// Interrupt mask
#define REG_INT_MASK        (12)
// DMA Src Addr
#define REG_DMA_SRC_ADDR    (16)
// DMA Total size
#define REG_DMA_TOTAL_SIZE  (20)
// DMA Src Size
#define REG_DMA_SRC_SIZE    (24)
// DMA consistent buffer Address
#define REG_CDMA_ADDR       (28)
// DMA consistent buffer length
#define REG_CDMA_LEN        (32)

#include <linux/ioctl.h>

// IOCTL用コマンドのタイプ
#define VPCI_IOC_TYPE 'V'

// レジスタに値を書き込む（デバッグ用）
#define VPCI_IOC_DBGWRITE \
    _IOW(VPCI_IOC_TYPE, 1, VpciIoCtlParam)

// レジスタから値を読み込む（デバッグ用）
#define VPCI_IOC_DBGREAD \
    _IOR(VPCI_IOC_TYPE, 3, VpciIoCtlParam)

// IOCTRLパラメータ
typedef struct {
    uint32_t *wdata;
    uint32_t *rdata;
    uint32_t wdataNum; // 4byteデータの数(NOT Bytes!)
    uint32_t rdataNum; // 4byteデータの数(NOT Bytes!)
} VpciIoCtlParam;

#endif // _VPCI_DRV_H_

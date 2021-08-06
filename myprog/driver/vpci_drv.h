#ifndef _VPCI_DRV_H_
#define _VPCI_DRV_H_

#include <linux/types.h>

/*
 * 本番ではレジスタ定義をユーザー空間に公開することはないが、検討段階では
 * デバッグをやりやすくするためにレジスタのオフセットを共有する。
 */
//--- 仮想レジスタアドレス定義 ---
// Version (R)
#define REG_VERSION         (0)
// Contol (W) x
#define REG_CTRL            (4)
// Interrupt status (R)
#define REG_INT_STATUS      (8)
// Interrupt mask (R/W) x
#define REG_INT_MASK        (12)
// DMA Src Addr (R/W) x
#define REG_DMA_SRC_ADDR    (16)
// DMA Dst Addr (R/W) x
#define REG_DMA_DST_ADDR    (20)
// DMA Total size (R/W) x
#define REG_DMA_SIZE        (24)
// Interrupt clear (W) x
#define REG_INT_CLEAR       (28)

#include <linux/ioctl.h>

// IOCTL用コマンドのタイプ
#define VPCI_IOC_TYPE 'V'

// HWバージョンを取得する
#define VPCI_IOC_GET_VERSION \
    _IOR(VPCI_IOC_TYPE, 1, VpciIoCtlParam)

// 2倍処理を起動する
#define VPCI_IOC_KICK_MUL2 \
    _IOWR(VPCI_IOC_TYPE, 3, VpciIoCtlParam)

// 4倍処理を起動する
#define VPCI_IOC_KICK_MUL4 \
    _IOWR(VPCI_IOC_TYPE, 4, VpciIoCtlParam)

// レジスタに値を書き込む
#define VPCI_IOC_WRITE_REG \
    _IOW(VPCI_IOC_TYPE, 5, VpciIoCtlParam)

// レジスタから値を読み込む
#define VPCI_IOC_READ_REG \
    _IOR(VPCI_IOC_TYPE, 6, VpciIoCtlParam)

// SW Reset
#define VPCI_IOC_RESET \
    _IOW(VPCI_IOC_TYPE, 7, VpciIoCtlParam)

// レジスタに値を書き込む（デバッグ用）
#define VPCI_IOC_DBGWRITE \
    _IOW(VPCI_IOC_TYPE, 100, VpciIoCtlParam)

// レジスタから値を読み込む（デバッグ用）
#define VPCI_IOC_DBGREAD \
    _IOR(VPCI_IOC_TYPE, 101, VpciIoCtlParam)

#define VPCI_IOC_DOINT \
    _IOW(VPCI_IOC_TYPE, 102, VpciIoCtlParam)

// IOCTRLパラメータ
typedef struct {
    // --- レジスタ R/W ---
    // レジスタアドレス(Offset)
    uint32_t addr;
    // 「設定する」または「取得した」レジスタ値
    uint32_t *val;

    // --- 2倍/4倍処理 ---
    // HWへ渡すデータ
    uint32_t *inData;
    // HWから受け取るデータ
    uint32_t *outData;
    // 処理するデータ数
    uint32_t dataNum;

    // --- デバッグ用 ---
    uint32_t *wdata;
    uint32_t *rdata;
    uint32_t wdataNum;
    uint32_t rdataNum;
} VpciIoCtlParam;

#endif // _VPCI_DRV_H_

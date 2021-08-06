#ifndef _VIRT_PCI_H_
#define _VIRT_PCI_H_

// ログを有効にする時は定義
#define LOG_LEVEL (2)

#define DEBUG_PRINT(fmt, ...)
#define INFO_PRINT(fmt, ...)
#define ERROR_PRINT(fmt, ...)

#if LOG_LEVEL >= 0
# undef ERROR_PRINT
# define ERROR_PRINT(fmt, ...) printf("[ERROR]%s/L.%d:" \
                                      fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#endif // LOG_LEVEL >=0

#if LOG_LEVEL >= 1
# undef INFO_PRINT
# define INFO_PRINT(fmt, ...) printf("[INFO]%s/L.%d:" \
                                     fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#endif // LOG_LEVEL >= 1

#if LOG_LEVEL >= 2
# undef DEBUG_PRINT
# define DEBUG_PRINT(fmt, ...) printf("[DEBUG]%s/L.%d:" \
                                      fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)

#endif // LOG_LEVEL >= 2



//--- MMIOとPIO領域サイズ(256 byte)
#define VIRT_PCI_MMIO_MEMSIZE (0x100)
#define VIRT_PCI_PIO_MEMSIZE  (0x100)

//--- DMAで使う内部バッファサイズ(4096 byte)
#define VIRT_PCI_DMA_BUFFER_SIZE (0x1000)

//--- PCIのベンダーIDとデバイスID ----
#define VIRT_PCI_VENDER_ID (0x1234)
#define VIRT_PCI_DEVICE_ID (0x0001)

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

//--- Controlレジスタビットフィールド取り出し ---
#define CTRL_KICK_2(r)       ((r)&0x1) // 2倍処理キック
#define CTRL_KICK_4(r)       ((r)&0x2) // 4倍処理キック
#define CTRL_RESET(r)        ((r)&0x4) // Reset
#define CTRL_DOINT(r)        ((r)&0x8) // 意味なく割り込み上げる

//--- Interrupt要因フラグ ---
#define INT_FINISH_2         (1<<0) // 2倍処理完了
#define INT_FINISH_4         (1<<1) // 4倍処理完了
#define INT_DOINT            (1<<3)

//--- Interrupt要因マスク ---
#define INT_FINISH_2_CLEAR    (~INT_FINISH_2)
#define INT_FINISH_4_CLEAR    (~INT_FINISH_4)

//--- mmioとpioのBAR no
#define MMIO_BAR    (0)
#define PIO_BAR     (1)

#endif // _VIRT_PCI_H_

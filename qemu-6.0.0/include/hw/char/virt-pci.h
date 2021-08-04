#ifndef _VIRT_PCI_H_
#define _VIRT_PCI_H_

#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, ...) printf("[DEBUG]%s/L.%d:" fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)

#undef INFO_PRINT
#define INFO_PRINT(fmt, ...) printf("[INFO]%s/L.%d:" fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)

#undef ERROR_PRINT
#define ERROR_PRINT(fmt, ...) printf("[ERROR]%s/L.%d:" fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)

//--- MMIOとPIO領域サイズ(256 byte)
#define VIRT_PCI_MMIO_MEMSIZE (0x100)
#define VIRT_PCI_PIO_MEMSIZE  (0x100)

//--- DMAで使う内部バッファサイズ(4096 byte)
#define VIRT_PCI_DMA_BUFFER_SIZE (0x1000)

//--- PCIのベンダーIDとデバイスID ----
#define VIRT_PCI_VENDER_ID (0x1234)
#define VIRT_PCI_DEVICE_ID (0x0001)

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

//--- Controlレジスタビットフィールド取り出し ---
#define CTRL_KICK(r)        ((r)&0x1)
#define CTRL_RESERVED(r)    (((r)&~0x1)>>1)

//--- Interrupt要因フラグ ---
#define INT_CONVERT_FINISH      (1<<0)
#define INT_CONVERT_CONTINUE    (1<<1)

//--- Interrupt要因マスク ---
#define INT_CONVERT_FINISH_MASK      (~INT_CONVERT_FINISH)
#define INT_CONVERT_CONTINUE_MASK    (~INT_CONVERT_CONTINUE)

//--- mmioとpioのBAR no
#define MMIO_BAR    (0)
#define PIO_BAR     (1)

#endif // _VIRT_PCI_H_

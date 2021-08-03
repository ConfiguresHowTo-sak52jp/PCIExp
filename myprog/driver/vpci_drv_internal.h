#ifndef _VPCI_DRV_INTERNAL_H_
#define _VPCI_DRV_INTERNAL_H_

// ----- LOGGER ------
#ifdef DEBUG_PRINT
#  undef DEBUG_PRINT
#endif // DEBUG_PRINT
#define DEBUG_PRINT(fmt, ...)

#ifdef INFO_PRINT
#  undef INFO_PRINT
#endif // INFO_PRINT
#define INFO_PRINT(fmt, ...)

#ifdef ERROR_PRINT
#  undef ERROR_PRINT
#endif // ERROR_PRINT
#define ERROR_PRINT(fmt, ...)

#ifdef LOG_LEVEL
#  if LOG_LEVEL >= 0
#    undef DEBUG_PRINT
#    define DEBUG_PRINT(fmt, ...) \
    printk(KERN_DEBUG "[DEBUG]%s/L.%d:" fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#  endif // LOG_LEVEL >= 0

#  if LOG_LEVEL >= 1
#    undef INFO_PRINT
#    define INFO_PRINT(fmt, ...) \
    printk(KERN_INFO "[INFO]%s/L.%d:" fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#  endif // LOG_LEVEL >= 1

#  if LOG_LEVEL >= 2
#    undef ERROR_PRINT
#    define ERROR_PRINT(fmt, ...) \
    printk(KERN_ERR "[ERROR]%s/L.%d:" fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)
#  endif // LOG_LEVEL >= 2
#endif // LOG_LEVEL

//--- 関数入退場記録 ---
#define FENTER DEBUG_PRINT("Enter\n")
#define FEXIT DEBUG_PRINT("Exit\n")

//--- レジスタアクセサ ---
#define ADDR2UINT uint32_t
#ifdef arm64
#  undef ADDR2UINT
#  define ADDR2UINT uint64_t
#endif

#define REG_READ(addr) *((volatile uint32_t*)(addr))
#define REG_WRITE(addr, val) do {*((volatile uint32_t*)(addr)) = val;} while(0)

//--- MMIOとPIO領域サイズ(256 byte)
#define VIRT_PCI_MMIO_MEMSIZE (0x100)
#define VIRT_PCI_PIO_MEMSIZE  (0x100)

//--- DMAで使う内部バッファサイズ(4096 byte)
#define VIRT_PCI_CDMA_BUFFER_SIZE (0x1000)
#define VIRT_PCI_SDMA_BUFFER_SIZE (0x1000)

//--- PCIのベンダーIDとデバイスID ----
#define VIRT_PCI_VENDER_ID (0x1234)
#define VIRT_PCI_DEVICE_ID (0x0001)

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

#endif // _VPCI_DRV_INTERNAL_H_

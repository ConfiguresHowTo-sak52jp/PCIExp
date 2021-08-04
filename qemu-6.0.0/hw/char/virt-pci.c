/*
 * PCI Virtual Device
 * 下記の仕様を持つ仮想デバイスである
 * 1.一つのpio領域と1つのmmio領域を持つ
 * 2.mmio領域は32ビットアドレス空間へマッピングされる、レジスタ空間である(unprefetchable)。
 * 3.coherent領域をバッファとして入力データをDMAでリードする機能を持つ。
 *   バッファからの読み込みが終わるたびに割り込みを発生し、設定したサイズを全て
 *   読み込むまで割り込みを上げる。全て完了した時のステータスと、途中のステータスは
 *   区別する。
 */

#include "qemu/osdep.h"
#include "hw/pci/pci.h"
#include "hw/qdev-properties.h"
#include "qemu/event_notifier.h"
#include "qemu/module.h"
#include "sysemu/kvm.h"
#include "qom/object.h"
#include "hw/qdev-properties-system.h"
#include "migration/vmstate.h"

#include "hw/char/virt-pci.h"

typedef struct {
    PCIDevice parent_obj;

    // レジスタ値
    uint32_t version;
    uint32_t ctrl;
    uint32_t int_status;
    uint32_t int_mask;
    uint32_t dma_src_addr;
    uint32_t dma_src_size;
    uint32_t dma_total_size;

    // Memory/IO
    MemoryRegion mmio;
    MemoryRegion portio;

    // Interrupt resource
    qemu_irq irq;
    
    // for DMA operation
    //   使うかどうか分からないがデバッグ用に確保しておく
    dma_addr_t cdma_addr;
    dma_addr_t sdma_addr;
    uint32_t cdma_buf[VIRT_PCI_DMA_BUFFER_SIZE/4];
    uint32_t sdma_buf[VIRT_PCI_DMA_BUFFER_SIZE/4];
} VirtPciState;

#define TYPE_VIRT_PCI "virt-pci"

//--- PIO領域は何もしない設定とする ---
static uint64_t virt_pci_pio_read(void *opaque, hwaddr addr, unsigned size)
{
    VirtPciState *s = (VirtPciState*)opaque;
    

    return 0;
}

static void virt_pci_pio_write(void *opaque, hwaddr addr, uint64_t val,
                               unsigned size)
{
    VirtPciState *s = (VirtPciState*)opaque;
    PCIDevice *pdev = PCI_DEVICE(s);
}


/*
 * TODO 動作を定義する
 */
static void virt_pci_mmio_write(void *opaque, hwaddr addr, uint64_t val,
                                unsigned size)
{
    VirtPciState *s = (VirtPciState *)opaque;
    PCIDevice *pdev = PCI_DEVICE(s);

    DEBUG_PRINT("addr=0x%08x,val=0x%08x\n", (uint32_t)addr, (uint32_t)val);
	
    switch (addr) {
    case REG_VERSION:
        s->version = val;
        break;
    case REG_CTRL:
        s->ctrl = val;
        break;
    case REG_INT_STATUS:
        s->int_status = val;
        break;
    case REG_DMA_SRC_ADDR:
        s->dma_src_addr = val;
        break;
    case REG_DMA_TOTAL_SIZE:
        s->dma_total_size = val;
        break;
    case REG_DMA_SRC_SIZE:
        s->dma_src_size = val;
        break;
    defualt:
        ERROR_PRINT("不正なアドレスが指定された:%d\n", addr);
        break;
    }
}


/*
 * レジスタ値を返す
 */
static uint64_t virt_pci_mmio_read(void *opaque, hwaddr addr, unsigned size)
{
    VirtPciState *s = (VirtPciState *)opaque;
    
    DEBUG_PRINT("addr=0x%08x\n", (uint32_t)addr);

    switch (addr) {
    case REG_VERSION:
        return s->version;
    case REG_CTRL:
        return s->ctrl;
    case REG_INT_STATUS:
        return s->int_status;
    case REG_DMA_SRC_ADDR:
        return s->dma_src_size;
    case REG_DMA_TOTAL_SIZE:
        return s->dma_total_size;
    case REG_DMA_SRC_SIZE:
        return s->dma_src_size;
    }
    return 0;
}

//--- mmioの振る舞い定義 ---
static const MemoryRegionOps virt_pci_mmio_ops = {
    .write = virt_pci_mmio_write,
    .read = virt_pci_mmio_read,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
};

//--- PIOの振る舞い定義(実際は何もしない) ---
static const MemoryRegionOps virt_pci_pio_ops = {
    .read = virt_pci_pio_read,
    .write = virt_pci_pio_write,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .impl = {
        .min_access_size = 1,
        .max_access_size = 4,
    },
};

static void virt_pci_reset(VirtPciState *s)
{
    s->ctrl = 0;
    s->int_status = 0;
    s->int_mask = 0;
    s->dma_src_addr = 0;
    s->dma_src_size = 0;
    s->dma_total_size = 0;

    s->cdma_addr = 0;
    s->sdma_addr = 0;

    for (int i = 0; i < VIRT_PCI_DMA_BUFFER_SIZE/4; i++) {
        s->cdma_buf[i] = 0;
        s->sdma_buf[i] = 0;
    }
}

static void qdev_virt_pci_reset(DeviceState *dev)
{
    VirtPciState *s = DO_UPCAST(VirtPciState, parent_obj, dev);
    virt_pci_reset(s);
}

static void virt_pci_realize(PCIDevice *pdev, Error **err)
{
    VirtPciState *s = DO_UPCAST(VirtPciState, parent_obj, pdev);
  
    // 割り込みパラメータ
    pdev->config[PCI_INTERRUPT_PIN] = 1;
    s->irq = pci_allocate_irq(&s->parent_obj);
    DEBUG_PRINT("IRQ=%p\n", s->irq);

    // mmio領域とPIO領域を登録
    memory_region_init_io(&s->mmio, OBJECT(s), &virt_pci_mmio_ops, s,
                          "virt_pci_mmio", VIRT_PCI_MMIO_MEMSIZE*2);
    memory_region_init_io(&s->portio, OBJECT(s), &virt_pci_pio_ops, s,
                          "virt_pci_portio", VIRT_PCI_PIO_MEMSIZE*2);
    DEBUG_PRINT("mmio=%p,pio=%p\n", s->mmio, s->portio);

    // BARの設定
    pci_register_bar(pdev, MMIO_BAR,
                     PCI_BASE_ADDRESS_SPACE_MEMORY, &s->mmio);
    pci_register_bar(pdev, PIO_BAR ,
                     PCI_BASE_ADDRESS_SPACE_IO, &s->portio);

    // local parameter初期化
    virt_pci_reset(s);
}

static void virt_pci_exit(PCIDevice *pdev)
{
    VirtPciState *s = DO_UPCAST(VirtPciState, parent_obj, pdev);
    qemu_free_irq(s->irq);
    
}

static void virt_pci_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *k = PCI_DEVICE_CLASS(klass);

    k->realize = virt_pci_realize;
    k->exit = virt_pci_exit;
    k->vendor_id = VIRT_PCI_VENDER_ID;
    k->device_id = VIRT_PCI_DEVICE_ID;
    k->revision = 0x00;
    k->class_id = PCI_CLASS_OTHERS;

    dc->desc = "PCI Virtual Device";
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);

    dc->reset = qdev_virt_pci_reset;
}

static const TypeInfo virt_pci_info = {
    .name          = TYPE_VIRT_PCI,
    .parent        = TYPE_PCI_DEVICE,
    .instance_size = sizeof(VirtPciState),
    .class_init    = virt_pci_class_init,
    .interfaces = (InterfaceInfo[]) {
        { INTERFACE_CONVENTIONAL_PCI_DEVICE },
        { },
    },
};

static void virt_pci_register_types(void)
{
    type_register_static(&virt_pci_info);
}

type_init(virt_pci_register_types)

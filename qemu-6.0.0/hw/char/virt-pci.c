/*
 * PCI Virtual Device
 * 下記の仕様を持つ仮想デバイスである
 * 1.一つのpio領域と1つのmmio領域を持つ
 * 2.mmio領域は32ビットアドレス空間へマッピングされる、レジスタ空間である(unprefetchable)。
 * 3.自身がバスマスタとなりDMA転送を制御する。デモ機能として、下記の2つの機能を持つ。
 *  3-1.ドライバから入力バッファを取得し、DMAを使って内部バッファに取り込む。
 *      次に内部バッファ値を2倍して、DMAを用いて結果を出力バッファへ書き込む。一連の
 *      処理が終わったら割り込みを上げる。
 *  3-2.ドライバから入力バッファ及び出力バッファを取得する。入力バッファからDMAを使って
 *      内部バッファに取り込む。次に内部バッファ値を4倍して、DMAを用いて結果を出力
 *      バッファへ書き込む。一連の処理が終わったら割り込みを上げる。
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
#include "hw/irq.h"

#include "hw/char/virt-pci.h"

typedef struct {
    PCIDevice parent_obj;

    // レジスタ値
    uint32_t version;
    uint32_t ctrl;
    uint32_t int_status;
    uint32_t int_mask;

    // 全てのレジスタ値（未定義領域含）を配列で記憶する
    uint32_t reg_val[VIRT_PCI_MMIO_MEMSIZE/4];

    // Memory/IO
    MemoryRegion mmio;
    MemoryRegion portio;

    // Interrupt resource
    qemu_irq irq;
    
    // for DMA operation
    dma_addr_t dma_src_addr;
    dma_addr_t dma_dst_addr;
    uint32_t dma_size;
    uint32_t dma_buf[VIRT_PCI_DMA_BUFFER_SIZE/4];

} VirtPciState;

#define TYPE_VIRT_PCI "virt-pci"

//--- PIO領域は何もしない設定 ---
static uint64_t virt_pci_pio_read(void *opaque, hwaddr addr, unsigned size)
{
    DEBUG_PRINT("Nothing to do!\n");
    return 0xbeefbeef;
}

static void virt_pci_pio_write(void *opaque, hwaddr addr, uint64_t val,
                               unsigned size)
{
    DEBUG_PRINT("Nothing to do!\n");
}

static void multiple(int m, uint32_t *buf, uint32_t len)
{
    for (int i = 0; i < len; i++)
        buf[i] *= m;
}

/**
 * @brief HWの擬似的なリセット実行
 */
static void virt_pci_reset(VirtPciState *s)
{
    s->ctrl = 0;
    s->int_status = 0;
    s->int_mask = 0xffffffff;
    s->dma_src_addr = 0;
    s->dma_dst_addr = 0;
    s->dma_size = 0;

    memset(s->dma_buf, 0, sizeof(s->dma_buf));
    memset(s->reg_val, 0, sizeof(s->reg_val));
    s->reg_val[REG_INT_MASK/4] = s->int_mask;

    // versionレジスタをセット(ver.0x12345678)
    s->reg_val[0] = 0x12345678;
}

/**
 * @brief レジスタ書き込みに対する処理
 */
static void virt_pci_mmio_write(void *opaque, hwaddr addr, uint64_t val,
                                unsigned size)
{
    VirtPciState *s = (VirtPciState *)opaque;
    PCIDevice *pdev = PCI_DEVICE(s);

    DEBUG_PRINT("addr=0x%08x,val=0x%08x\n", (uint32_t)addr, (uint32_t)val);

    // R onlyの時は何もしない
    switch (addr) {
    case REG_VERSION:
    case REG_INT_STATUS:
        return;
    }

    // まず一様に履歴記録。処理結果に応じて変更する場合は後で上書き
    if (addr < VIRT_PCI_MMIO_MEMSIZE)
        s->reg_val[addr/4] = val;

    switch (addr) {
    case REG_CTRL:
        // 2倍処理キック
        if (CTRL_KICK_2(val)) {
            // Buffer1からDMA read->2倍する->Buffer2にDMA write
            pci_dma_read(pdev, s->dma_src_addr, s->dma_buf, s->dma_size);
            DEBUG_PRINT("pci_dma_read() success:src=%lx,dst=%lx,size=%u\n",
                        (uint64_t)s->dma_src_addr,
                        (uint64_t)s->dma_buf,
                        s->dma_size
                );
            multiple(2, s->dma_buf, s->dma_size/4);
            DEBUG_PRINT("multiple() success\n");
            pci_dma_write(pdev, s->dma_dst_addr, s->dma_buf, s->dma_size);
            DEBUG_PRINT("pci_dma_write() success:src=%lx,dst=%lx,size=%u\n",
                        (uint64_t)s->dma_buf,
                        (uint64_t)s->dma_dst_addr,
                        s->dma_size
                );
            // 割り込みマスクされていなかったら割り込み上げる
            s->int_status |= INT_FINISH_2;
            s->reg_val[REG_INT_STATUS/4] = s->int_status;
            DEBUG_PRINT("int_status=%08x,int_mask=%08x\n", s->int_status, s->int_mask);
            if (INT_FINISH_2 & s->int_mask) {
                pci_irq_assert(pdev);
                DEBUG_PRINT("pci_irq_assert() success\n");
            }
        }
        // 4倍処理キック
        else if (CTRL_KICK_4(val)) {
            // Buffer1からDMA read->4倍する->Buffer2にDMA write
            pci_dma_read(pdev, s->dma_src_addr, s->dma_buf, s->dma_size);
            multiple(4, s->dma_buf, s->dma_size/4);
            pci_dma_write(pdev, s->dma_dst_addr, s->dma_buf, s->dma_size);
            // 割り込みマスクされていなかったら割り込み上げる
            s->int_status |= INT_FINISH_4;
            s->reg_val[REG_INT_STATUS/4] = s->int_status;
            if (INT_FINISH_4 & s->int_mask)
                pci_irq_assert(pdev);
        }
        // SW Reset
        else if (CTRL_RESET(val)) {
            virt_pci_reset(s);
        }
        else if (CTRL_DOINT(val)) {
            s->int_status |= INT_DOINT;
            s->reg_val[REG_INT_STATUS/4] = s->int_status;
            pci_irq_assert(pdev);
        }
        break;
    case REG_INT_CLEAR:
        // 有効なフラグでなかったら何もしない
        if (!(val & INT_FINISH_2) && !(val & INT_FINISH_4) && !(val & INT_DOINT)) {
            DEBUG_PRINT("Invalid flag:%08x", (uint32_t)val);
            break;
        }
        // 割り込み実行中でなければ何もしない
        if (!s->int_status) {
            DEBUG_PRINT("Not in interrupt state. Nothing to do\n");
            break;
        }
        // 指定されたビットをクリアし要因が全て無くなったら割り込みを下げる
        s->int_status &= ~val;
        s->reg_val[REG_INT_STATUS/4] = s->int_status;
        if (s->int_status == 0)
            pci_irq_deassert(pdev);
        break;
    case REG_DMA_SRC_ADDR:
        s->dma_src_addr = val;
        break;
    case REG_DMA_SIZE:
        s->dma_size = val;
        break;
    case REG_DMA_DST_ADDR:
        s->dma_dst_addr = val;
        break;
    case REG_INT_MASK:
        // 有効なマスク値が設定されている時だけ処理
        if ((~val) & INT_FINISH_2 || (~val) & INT_FINISH_4 || (~val) & INT_DOINT) {
            s->int_mask = val;
        }
        else {
            DEBUG_PRINT("Illegal val to REG_INT_MASK:%08x\n", (uint32_t)val);
        }
        break;
    }
}


/**
 * @brief レジスタ値を返す
 */
static uint64_t virt_pci_mmio_read(void *opaque, hwaddr addr, unsigned size)
{
    DEBUG_PRINT("addr=0x%08x\n", (uint32_t)addr);

    VirtPciState *s = (VirtPciState *)opaque;
    uint64_t ret = 0;
    if (addr < VIRT_PCI_MMIO_MEMSIZE)
        ret = s->reg_val[addr/4];

    // W onlyの時は 0xbeefbeef を返す
    switch (addr) {
    case REG_CTRL:
    case REG_INT_CLEAR:
        ret = 0xbeefbeef;
        break;
    }
    
    return ret;
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


static void qdev_virt_pci_reset(DeviceState *dev)
{
    VirtPciState *s = DO_UPCAST(VirtPciState, parent_obj,
                                DO_UPCAST(PCIDevice, qdev, dev));
    virt_pci_reset(s);
}

static void virt_pci_realize(PCIDevice *pdev, Error **err)
{
    VirtPciState *s = DO_UPCAST(VirtPciState, parent_obj, pdev);
  
    // 割り込みパラメータ
    //pdev->config[PCI_INTERRUPT_LINE] = 0;
    pdev->config[PCI_INTERRUPT_PIN] = 1;
    //s->irq = pci_allocate_irq(&s->parent_obj);
    DEBUG_PRINT("IRQ=%p\n", s->irq);

    // mmio領域とPIO領域を登録
    memory_region_init_io(&s->mmio, OBJECT(s), &virt_pci_mmio_ops, s,
                          "virt_pci_mmio", VIRT_PCI_MMIO_MEMSIZE*2);
    memory_region_init_io(&s->portio, OBJECT(s), &virt_pci_pio_ops, s,
                          "virt_pci_portio", VIRT_PCI_PIO_MEMSIZE*2);
 
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
    //qemu_free_irq(s->irq);
    
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

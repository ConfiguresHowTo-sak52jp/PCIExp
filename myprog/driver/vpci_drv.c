#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/dma-mapping.h>
#include <linux/irqreturn.h>

#include "vpci_drv.h"
#include "vpci_drv_internal.h"

// デバイスに関する情報
MODULE_LICENSE("Dual BSD/GPL");

// /proc/devices等で表示されるデバイス名
#define DRIVER_NAME "vpci"

// マイナー番号の開始番号とデバイス数 */
static const unsigned int MINOR_BASE = 0;
static const unsigned int MINOR_NUM  = 1;

// メジャー番号(動的に決める)
static unsigned int vpci_major;

// キャラクタデバイスのオブジェクト
static struct cdev vpci_cdev;

// デバイスドライバのクラスオブジェクト
static struct class *vpci_class = NULL;

// このドライバが管理するデータ構造
typedef struct {
    struct pci_dev *pdev;
    struct cdev *cdev;

    uint32_t vendor_id;
    uint32_t device_id;
            
    // pio関連
    //  base:BARに設定されたBase
    uint32_t pio_base;
    uint32_t pio_len;
    uint32_t pio_flag;

    // mmio関連
    //  mmio_base:BARに設定されたBase
    //  mmio_addr:ioremap後のアドレス
    uint32_t mmio_base;
    uint32_t mmio_len;
    uint32_t mmio_flag;
    uint8_t *mmio_addr;
    
    // DMA関連
    dma_addr_t cdma_addr;
    dma_addr_t sdma_addr;
    void *cdma_buff;
    void *sdma_buff;
    wait_queue_head_t sdma_q;
    
} VirtPciData_t;

// インスタンス
static VirtPciData_t *virtPciData = NULL;

/**
 * @brief ioctlハンドラ
 */
static long vpci_ioctl(
    struct file *filp,
    unsigned int cmd,
    unsigned long arg)
{
    FENTER;
    long ret = 0;
    
    VpciIoCtlParam *up = (VpciIoCtlParam *)arg;
    VirtPciData_t *d = filp->private_data;

    VpciIoCtlParam *kp = kmalloc(sizeof(VpciIoCtlParam), GFP_KERNEL);
    uint32_t *wdata = NULL;
    uint32_t *rdata = NULL;
    ADDR2UINT base = (ADDR2UINT)d->mmio_addr;
    uint32_t *addr = NULL;
    switch (cmd) {
        case VPCI_IOC_DBGWRITE:
            DEBUG_PRINT("VPCI_IOC_DBGWRITE\n");
            if (copy_from_user(kp, up, sizeof(VpciIoCtlParam)) == 0) {
                ERROR_PRINT("copy_from_user() failed");
                ret = -EFAULT;
                goto END;
            }
            // 書き込みデータを取得
            wdata = kmalloc(kp->wdataNum*sizeof(uint32_t), GFP_KERNEL);
            if (copy_from_user(wdata, kp->wdata, kp->wdataNum*sizeof(uint32_t)) == 0) {
                ERROR_PRINT("copy_from_user() failed");
                ret = -EFAULT;
                goto END;
            }
            // wdata[0]=addr, wdata[1..wdataNum-1]=val
            DEBUG_PRINT("RegOffset=%u,Num=%u\n", wdata[0], kp->wdataNum-1);
            addr = (uint32_t*)(base+wdata[0]);
            for (int i = 1; i < kp->wdataNum; i++)
                REG_WRITE(addr++, wdata[i]);
            kfree(wdata); wdata = NULL;
            break;
        case VPCI_IOC_DBGREAD:
            DEBUG_PRINT("VPCI_IOC_DBGREAD\n");
            if (copy_from_user(kp, up, sizeof(VpciIoCtlParam)) == 0) {
                ERROR_PRINT("copy_from_user() failed");
                ret = -EFAULT;
                goto END;
            }
            // 読み込みデータをレジスタから取得
            rdata = kmalloc(kp->rdataNum*sizeof(uint32_t), GFP_KERNEL);
            uint32_t regAddr = 0;
            if (copy_from_user(&regAddr, kp->wdata, sizeof(uint32_t)) == 0) {
                ERROR_PRINT("copy_from_user() failed");
                ret = -EFAULT;
                goto END;
            }
            DEBUG_PRINT("RegOffset=%u,Num=%u\n", regAddr, kp->rdataNum);
            addr = (uint32_t*)(base+regAddr);
            for (int i = 0; i < kp->rdataNum; i++)
                rdata[i] = REG_READ(addr++);
            if (copy_to_user(kp->rdata, rdata, kp->rdataNum*sizeof(uint32_t)) == 0) {
                ERROR_PRINT("copy_to_user() failed");
                ret = -EFAULT;
                goto END;
            }
            kfree(rdata); rdata = NULL;
            break;
        default:
            ret = -EFAULT;
            break;
    }
END:
    kfree(kp);
    kfree(rdata);
    kfree(wdata);
    
    FEXIT;
    return 0;
}


/* open時に呼ばれる関数 */
static int vpci_open(struct inode *inode, struct file *file)
{
    FENTER;
    file->private_data = virtPciData;
    FEXIT;
    return 0;
}

/* close時に呼ばれる関数 */
static int vpci_close(struct inode *inode, struct file *file)
{
    FENTER;

    file->private_data = NULL;
    
    FEXIT;
    return 0;
}

/* read時に呼ばれる関数 */
static ssize_t vpci_read(
    struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    FENTER;

    FEXIT;
    return count;
}

/* write時に呼ばれる関数 */
static ssize_t vpci_write(
    struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    FENTER;

    FEXIT;
    return count;
}


/* 各種システムコールに対応するハンドラテーブル */
static struct file_operations vpci_fops = {
    .open    = vpci_open,
    .release = vpci_close,
    .read    = vpci_read,
    .write   = vpci_write,
    .unlocked_ioctl = vpci_ioctl,
    .compat_ioctl   = vpci_ioctl,   // for 32-bit App
};

// ----- PCI driverとしての振る舞い ------

// このドライバが面倒見るデバイスの識別子を配列で与える
static struct pci_device_id vpci_pci_ids[] = {
    {PCI_DEVICE(VIRT_PCI_VENDER_ID, VIRT_PCI_DEVICE_ID)},
    {0,}
};

MODULE_DEVICE_TABLE(pci, vpci_pci_ids);

// 割り込みハンドラ定義
static irqreturn_t vpci_irq_handler(int irq, void *dev_id)
{
    return IRQ_HANDLED;
}


static int vpci_pci_probe(struct pci_dev *pdev,
                          const struct pci_device_id *id)
{
    FENTER;
    
    virtPciData = kmalloc(sizeof(VirtPciData_t), GFP_KERNEL);
    if (virtPciData == NULL) {
        ERROR_PRINT("kmalloc() failed\n");
        return -ENOMEM;
    }
    memset(virtPciData, 0, sizeof(VirtPciData_t));
    
    // PCIデバイスを有効にする
    int ret = pci_enable_device(pdev);
    if (ret) {
        ERROR_PRINT("pci_enable_device() failed\n");
        ret = -EBUSY;
        goto END_01;
    }
    DEBUG_PRINT("pci_enable_device() success!\n");

    // PCI領域の確保
    for (int i = 0; i < 2; i++) {
        ret = pci_request_region(pdev, i, DRIVER_NAME);
        if (ret) {
            ERROR_PRINT("pci_request_region() for BAR%d failed\n", i);
            goto END_10;
        }
    }
    DEBUG_PRINT("pci_request_region() success!\n");
    
    // mmio領域の設定
    virtPciData->mmio_base = pci_resource_start(pdev, MMIO_BAR);
    virtPciData->mmio_len = pci_resource_len(pdev, MMIO_BAR);
    virtPciData->mmio_flag = pci_resource_flags(pdev, MMIO_BAR);
    virtPciData->mmio_addr = ioremap(virtPciData->mmio_base,
                                     virtPciData->mmio_len);
    DEBUG_PRINT("mmio setting success!\n");
    DEBUG_PRINT("base=0x%08x,len=%d,addr=%p\n",
                virtPciData->mmio_base,
                virtPciData->mmio_len,
                virtPciData->mmio_addr);

    // pio領域の設定
    virtPciData->pio_base = pci_resource_start(pdev, PIO_BAR);
    virtPciData->pio_len = pci_resource_len(pdev, PIO_BAR);
    virtPciData->pio_flag = pci_resource_flags(pdev, PIO_BAR);
    DEBUG_PRINT("pio setting success!\n");
    DEBUG_PRINT("base=0x%08x,len=%d\n",
                virtPciData->pio_base,
                virtPciData->pio_len);

    // MISC datas
    uint16_t id1;
    pci_read_config_word(pdev, PCI_VENDOR_ID, &id1);
    virtPciData->vendor_id = id1;
    pci_read_config_word(pdev, PCI_DEVICE_ID, &id1);
    virtPciData->device_id = id1;
    virtPciData->pdev = pdev;
    virtPciData->cdev = &vpci_cdev;

    // 割り込みハンドラ設定
    ret = request_irq(pdev->irq, vpci_irq_handler,
                      0, DRIVER_NAME, pdev);

    if (ret) {
        ERROR_PRINT("request_irq() failed!\n");
        goto END_20;
    }

    // DMAの設定
    ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
    if (ret) {
        ERROR_PRINT("pci_set_dma_mask() failed!\n");
        goto END_30;
    }
    pci_set_master(pdev);

    // DMAで使うコヒーレントバッファ確保
    virtPciData->cdma_buff = dma_alloc_coherent(
        &pdev->dev, VIRT_PCI_CDMA_BUFFER_SIZE,
        &virtPciData->cdma_addr, GFP_KERNEL);
    if (virtPciData->cdma_buff == NULL) {
        ERROR_PRINT("dma_alloc_coherent() failed!\n");
        ret = -ENOMEM;
        goto END_40;
    }
    DEBUG_PRINT("cdma_buff=0x%p,cdma_addr=0x%08llx\n",
                virtPciData->cdma_buff,
                virtPciData->cdma_addr);
    
    // DMAで使うストリームバッファ確保
    virtPciData->sdma_buff =
            kmalloc(VIRT_PCI_SDMA_BUFFER_SIZE, GFP_KERNEL);
    if (virtPciData->sdma_buff == NULL) {
        ERROR_PRINT("kmaolloc() failed!\n");
        ret = -ENOMEM;
        goto END_50;
    }

    // DMAのwaitqueue初期化
    init_waitqueue_head(&virtPciData->sdma_q);
        
END_50:
    if (ret)
        dma_free_coherent(
            &pdev->dev, VIRT_PCI_CDMA_BUFFER_SIZE, virtPciData->cdma_buff,
            virtPciData->cdma_addr);

END_40:
    if (ret)
        pci_clear_master(pdev);

END_30:
    if (ret)
        free_irq(pdev->irq, pdev);
        
END_20:
    if (ret) {
        iounmap(virtPciData->mmio_addr);
        pci_release_regions(pdev);
    }

END_10:
    if (ret)
        pci_disable_device(pdev);
    
END_01:
    if (ret) {
        kfree(virtPciData);
        virtPciData = NULL;
    }

    FEXIT;
    return ret;
}

static void vpci_pci_remove(struct pci_dev *pdev)
{
    FENTER;
    
    kfree(virtPciData->sdma_buff);
    dma_free_coherent(
        &pdev->dev, VIRT_PCI_CDMA_BUFFER_SIZE, virtPciData->cdma_buff,
        virtPciData->cdma_addr);
    pci_clear_master(pdev);
    free_irq(pdev->irq, pdev);
    iounmap(virtPciData->mmio_addr);
    pci_release_regions(pdev);
    pci_disable_device(pdev);
    kfree(virtPciData);
    virtPciData = NULL;

    FEXIT;
}

static struct pci_driver vpci_pci_driver =
{
	.name = DRIVER_NAME,
	.id_table = vpci_pci_ids,
	.probe = vpci_pci_probe,
	.remove = vpci_pci_remove,
};


/* ロード(insmod)時に呼ばれる関数 */
static int vpci_init(void)
{
    FENTER;

    int alloc_ret = 0;
    int cdev_err = 0;
    dev_t dev;
    int ret = 0;
    
    // 空いているメジャー番号を確保する
    alloc_ret = alloc_chrdev_region(&dev, MINOR_BASE, MINOR_NUM, DRIVER_NAME);
    if (alloc_ret != 0) {
        ERROR_PRINT("alloc_chrdev_region = %d\n", alloc_ret);
        ret = -EBUSY;
        goto END_01;
    }

    // 取得したdev( = メジャー番号 + マイナー番号)からメジャー番号を取得して保持しておく
    vpci_major = MAJOR(dev);
    dev = MKDEV(vpci_major, MINOR_BASE);

    // cdev構造体の初期化とシステムコールハンドラテーブルの登録
    cdev_init(&vpci_cdev, &vpci_fops);
    vpci_cdev.owner = THIS_MODULE;

    // このデバイスドライバ(cdev)をカーネルに登録する
    cdev_err = cdev_add(&vpci_cdev, dev, MINOR_NUM);
    if (cdev_err != 0) {
        ERROR_PRINT("cdev_add = %d\n", cdev_err);
        ret = -EBUSY;
        goto END_77;
    }

    // このデバイスのクラス登録をする(/sys/class/vpci/ を作る)
    vpci_class = class_create(THIS_MODULE, DRIVER_NAME);
    if (IS_ERR(vpci_class)) {
        ERROR_PRINT("class_create() failed\n");
        ret = -EBUSY;
        goto END_88;
    }

    // /sys/class/vpci/vpci* を作る
    for (int minor = MINOR_BASE; minor < MINOR_BASE + MINOR_NUM; minor++) {
        device_create(vpci_class,
                      NULL,
                      MKDEV(vpci_major, minor),
                      NULL,
                      "vpci%d", minor);
    }

    // PCIドライバとして登録する
    ret = pci_register_driver(&vpci_pci_driver);
    if (ret) {
        ERROR_PRINT("pci_register_driver() failed:%d\n", ret);
        goto END_99;
    }

END_99:
    if (ret) {
        for (int minor = MINOR_BASE; minor < MINOR_BASE + MINOR_NUM; minor++) {
            device_destroy(vpci_class, MKDEV(vpci_major, minor));
        }
        class_destroy(vpci_class);
    }

END_88:
    if (ret) {
        cdev_del(&vpci_cdev);
    }

END_77:
    if (ret)
        unregister_chrdev_region(dev, MINOR_NUM);
    
END_01:
    FEXIT;
    return ret;
}

/* アンロード(rmmod)時に呼ばれる関数 */
static void vpci_exit(void)
{
    FENTER;

    // --- PCIドライバから取り除く ---
    pci_unregister_driver(&vpci_pci_driver);

    // --- キャラドライバから取り除く ---
    dev_t dev = MKDEV(vpci_major, MINOR_BASE);
    
    // /sys/class/vpci/vpci* を削除する
    for (int minor = MINOR_BASE; minor < MINOR_BASE + MINOR_NUM; minor++) {
        device_destroy(vpci_class, MKDEV(vpci_major, minor));
    }

    // このデバイスのクラス登録を取り除く(/sys/class/vpci/を削除する)
    class_destroy(vpci_class);

    // このデバイスドライバ(cdev)をカーネルから取り除く
    cdev_del(&vpci_cdev);

    // このデバイスドライバで使用していたメジャー番号の登録を取り除く
    unregister_chrdev_region(dev, MINOR_NUM);

    FEXIT;
}

module_init(vpci_init);
module_exit(vpci_exit);

#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/pci_regs.h>

#define MY_DRIVER   "my_pci_driver"
#define NUM_MSI_VECTORS 3 

static struct pci_device_id my_driver_id_table[] = {
    {PCI_DEVICE(0X010F, 0X0F0E)},
    {0,}
}; 

static int my_driver_probe(struct pci_dev *pdev, const struct pci_device_id *ent); 
static void my_driver_remove(struct pci_dev *pdev); 

static struct pci_driver my_driver = {
    .name = MY_DRIVER, 
    .id_table = my_driver_id_tablem 
    .probee = my_driver_probe, 
    .remove = my_driver_remove
}; 


struct my_driver_priv {
    u8 __iomem *hwmem; 
}; 

static int __init mypci_driver_init(void)
{
    return pci_register_driver(&my_driver); 
}

static void __exit mypci_driver_exit(void)
{
    pci_unregister_driver(&my_driver); 
}

void release_device(struct pci_dev *pdev)
{
    free_irq(42, pdev); 
    pci_release_region(pdev, pci_select_bars(pdev, IORESOURCE_MEM)); 
    pci_disable_device(pdev); 
}

static irqreturn_t irq_handler(int irq, void *cookie)
{
    (void)cookie; 
    printk("handle IRQ #%d\n", irq); 
    return IRQ_HANDLED; 
}

int set_interrupts(struct pci_dev *pdev)
{
     int i, nvecs, ret; 
     int *irq_vectors;

     nvecs = pci_alloc_irq_vectors(pdev, NUM_MSI_VECTORS, NUM_MSI_VECTORS, PCI_IRQ_MSI); 
     if(nvecs < 0)
     {
         pr_info("Failed to allocate MSI vectors\n"); 
         return nvecs; 
     }

     pr_info("Allocated %d MSI-x vectors\n", nvecs); 

     /*request one IRQ per vector*/ 
     for(i = 0; i < nvecs; ++i)
     {
         ret = request_irq(pci_irq_vector(pdev, i), irq_handler, 0, "my_pci_irq", pdev);
         if(ret)
         {
             pr_info("Failed to request IRQ vector %d\n", i); 
             goto err_free_vectrs 
         }
     }
     return 0; 

err_free_vectors:
     while(--i >= 0)
         free_irq(pci_irq_vector(pdev, i), pdev); 
     pci_free_irq_vectors(pdev); 
     return ret; 
}


void write_sample_data(struct pci_dev *pdev)
 {
     int data_to_write = 0xDEADBEEF; 

     struct my_driver_priv *drv_priv = (struct my_driver_priv *)pci_get_drvdata(pdev); 
        
     if(!drv_priv)
         return; 

     iowrite32(data_to_write, drv_priv->hwmem); 

}

static int my_driver_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
    int bar_mask, bar_index, err; 
    u16 vendor , device; 
    unsigned long mmio_start, mmio_len; 

    struct my_driver_priv *drv_priv; 

    pci_read_config_word(pdev, PCI_VENDOR_ID, &vendor); 
    pci_read_config_word(pdev, PCI_DEVICE_ID, &device); 

    pr_info("Device vid : 0x%X pid: 0x%X\n", vendor, device);
    
    err = pci_enable_device_mem(pdev); 
    if(err)
        return err; 


    /*request bar */ 
    bar_mask = pci_select_bars(pdev, IORESOURCE_MEM);
    if(!bar_mask)
    {
        pr_err("No memory BARS found\n"); 
        pci_disable_device(pdev); 
        return -ENODEV;
    }
    bar_index = ffs(bar_index) - 1; 


    err = pci_request_region(pdev, bar_index, MY_DRIVER); 
    if(err)
    {
        pr_err("Failed tp request BAR %d", bar_index); 
        pci_disable_device(pdev); 
        return err; 
    }   

    mmio_start = pci_resource_start(pdev, bar_index); 
    mmio_len = pci_resource_len(pdev, bar_index); 

    drv_priv = kzalloc(sizeof(struct my_driver_priv), GFP_KERNEL); 
    if(!drv_priv)
    {
        release_device(pdev); 
        return -ENOMEM; 
    }

    /*map bar memory region to kernel address space*/ 
    drv_priv->hwmem = ioremap(mmio_start, mmio_len);
    if(!drv_priv)
    {
        pr_err("memorymaping bar region failed\n"); 
        release_device(pdev); 
        return -EIO; 
    }

    pci_ser_drvdata(pdev, drv_priv); 

    write_sample_data(pdev); 

    return set_interrupts(pdev); 
}

static void my_driver_remove(struct pci_dev *pdev)
{
     struct my_driver_priv *drv_priv = pci_get_drvdata(pdev);

    if (drv_priv) {
        if (drv_priv->hwmem) {
            iounmap(drv_priv->hwmem);
        }

        pci_free_irq_vectors(pdev);

        kfree(drv_priv);
    }

    release_device(pdev);
}



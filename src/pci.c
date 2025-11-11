#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>


#define MY_DRIVER   "my_pci_driver"


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

}

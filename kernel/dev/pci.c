#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "memlayout.h"
#include "pci.h"

#define MAX_PCI_DEVICE_NUM 32

struct pci_dev_raw {
	uint32_t           id;
	volatile uint32_t* base;
};

static int dev_num = 0;
static struct pci_dev_raw pci_dev_raws[MAX_PCI_DEVICE_NUM];
static struct pci_dev* pci_devs[MAX_PCI_DEVICE_NUM];

static void pci_scan_bus();

void pci_init()
{
	memset(pci_dev_raws, 0, sizeof(pci_dev_raws));
	memset(pci_devs, 0, sizeof(pci_devs));
	pci_scan_bus();
}

static void pci_scan_bus() {
	int raw_dev_num = 0;
	for(int dev = 0; dev < MAX_PCI_DEVICE_NUM; dev++) {
		int bus = 0;
		int func = 0;
		int offset = 0;
		uint32_t off = (bus << 16) | (dev << 11) | (func << 8) | (offset);
		volatile uint32_t *base = (uint32_t *)ECAM + off;
		uint32_t id = base[0];

		if (id == -1)
			continue;

		pci_dev_raws[raw_dev_num].base = base;
		printf("id: %x, base: %p\n", id, base);
		pci_dev_raws[raw_dev_num].id = id;
		raw_dev_num += 1;
	}
}

int pci_register_device(struct pci_dev *dev) {
	for (int i = 0; i < MAX_PCI_DEVICE_NUM; i++) {
		if (pci_dev_raws[i].id == dev->id) {
			pci_devs[dev_num] = dev;
			dev->base = pci_dev_raws[i].base;
			if (dev->driver->init(dev) < 0) {
				return -1;
			}
			dev_num += 1;
			return 0;
		}
	}
	return -1;
}

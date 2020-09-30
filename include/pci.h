#pragma once

struct pci_dev {
	const char*                 name;
	uint32_t                    id;
	volatile uint32_t*          base;
	struct pci_driver*          driver;
};

struct pci_driver {
	const char* name;
	int (*init)(struct pci_dev* dev);
};
 
int pci_register_device(struct pci_dev *dev);

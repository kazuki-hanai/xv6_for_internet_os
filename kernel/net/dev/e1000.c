#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "trap.h"
#include "pci.h"
#include "net/dev/e1000_dev.h"
#include "net/dev/netdev.h"
#include "net/mbuf.h"

static void e1000_init_core(uint32_t* xregs);
static int e1000_driver_startup(struct pci_dev* dev);
static void e1000_alloc_netdev(uint32_t* xregs);
static int e1000_transmit(struct mbuf *m);
static void e1000_recv(void);
static void e1000_intr();

struct netdev* e1000ndev;

static struct pci_driver e1000_driver = {
	.name   = "82540EM Gigabit Ethernet Controller driver",
	.init = e1000_driver_startup
};
static struct pci_dev e1000_pcidev = {
	.name  = "82540EM Gigabit Ethernet Controller",
	.id    = ID_82540EM,
	.base  = 0,
	.driver = &e1000_driver
};

void pci_register_e1000() {
	pci_register_device(&e1000_pcidev);
}

static int e1000_driver_startup(struct pci_dev* dev) {
	dev->base[1] = 7;
	__sync_synchronize();

	for(int i = 0; i < 6; i++) {
		uint32_t old = dev->base[4+i];
		dev->base[4+i] = 0xffffffff;
		__sync_synchronize();
		dev->base[4+i] = old;
	}

	dev->base[4+0] = (uint32_t) E1000_REG;

	printf("e1000_init start\n");
	e1000_init_core((uint32_t *)E1000_REG);

	return 0;
}

static void e1000_init_core(uint32_t* xregs) {
	int i;

	e1000_alloc_netdev(xregs);
	if (e1000ndev == 0) {
		panic("cannot alloc ndev in e1000");
	}
	struct e1000_dev* e1000dev = GET_RAWDEV(e1000ndev);
	

	// Reset the device
	e1000dev->regs[E1000_IMS] = 0; // disable interrupts
	e1000dev->regs[E1000_CTL] |= E1000_CTL_RST;
	e1000dev->regs[E1000_IMS] = 0; // redisable interrupts
	__sync_synchronize();

	// [E1000 14.5] Transmit initialization
	memset(e1000dev->tx_ring, 0, sizeof(e1000dev->tx_ring));
	for (i = 0; i < TX_RING_SIZE; i++) {
		e1000dev->tx_ring[i].status = E1000_TXD_STAT_DD;
		e1000dev->tx_mbuf[i] = 0;
	}
	e1000dev->regs[E1000_TDBAL] = (uint64_t) e1000dev->tx_ring;
	if(sizeof(e1000dev->tx_ring) % 128 != 0)
		panic("e1000");
	e1000dev->regs[E1000_TDLEN] = sizeof(e1000dev->tx_ring);
	e1000dev->regs[E1000_TDH] = e1000dev->regs[E1000_TDT] = 0;
	
	// [E1000 14.4] Receive initialization
	memset(e1000dev->rx_ring, 0, sizeof(e1000dev->rx_ring));
	for (i = 0; i < RX_RING_SIZE; i++) {
		e1000dev->rx_ring[i].addr = (uint64_t) e1000dev->rx_mbuf[i]->buf;
	}
	e1000dev->regs[E1000_RDBAL] = (uint64_t) e1000dev->rx_ring;
	if(sizeof(e1000dev->rx_ring) % 128 != 0)
		panic("e1000");
	e1000dev->regs[E1000_RDH] = 0;
	e1000dev->regs[E1000_RDT] = RX_RING_SIZE - 1;
	e1000dev->regs[E1000_RDLEN] = sizeof(e1000dev->rx_ring);

	// filter by qemu's MAC address, 52:54:00:12:34:56
	e1000dev->regs[E1000_RA] = 0x12005452;
	e1000dev->regs[E1000_RA+1] = 0x5634 | (1<<31);
	// multicast table
	for (int i = 0; i < 4096/32; i++)
		e1000dev->regs[E1000_MTA + i] = 0;

	// transmitter control bits.
	e1000dev->regs[E1000_TCTL] = E1000_TCTL_EN |  // enable
		E1000_TCTL_PSP |                  // pad short packets
		(0x10 << E1000_TCTL_CT_SHIFT) |   // collision stuff
		(0x40 << E1000_TCTL_COLD_SHIFT);
	e1000dev->regs[E1000_TIPG] = 10 | (8<<10) | (6<<20); // inter-pkt gap

	// receiver control bits.
	e1000dev->regs[E1000_RCTL] = E1000_RCTL_EN | // enable receiver
		E1000_RCTL_BAM |                 // enable broadcast
		E1000_RCTL_SZ_2048 |             // 2048-byte rx buffers
		E1000_RCTL_SECRC;                // strip CRC
	
	// ask e1000 for receive interrupts.
	e1000dev->regs[E1000_RDTR] = 0; // interrupt after every received packet (no timer)
	e1000dev->regs[E1000_RADV] = 0; // interrupt after every packet (no timer)
	e1000dev->regs[E1000_IMS] = (1 << 7); // RXDW -- Receiver Descriptor Write Back
}

static void e1000_alloc_netdev(uint32_t* xregs) {
	struct e1000_dev* e1000dev;

	e1000ndev = ufkalloc(NETDEV_ALIGN(struct e1000_dev));
	e1000ndev->transmit = e1000_transmit;
	e1000ndev->recv = e1000_recv;
	e1000ndev->intr = e1000_intr;
	
	devintr_register_callback(E1000_IRQ, e1000ndev->intr);
	
	e1000dev = (struct e1000_dev*)GET_RAWDEV(e1000ndev);
	initlock(&e1000dev->lock, "e1000");
	e1000dev->regs = xregs;
	e1000dev->ndev = e1000ndev;
	e1000dev->pdev = &e1000_pcidev;

	for (int i = 0; i < RX_RING_SIZE; i++)
		e1000dev->rx_mbuf[i] = mbufalloc(0);
}

static int e1000_transmit(struct mbuf *m) {
	struct e1000_dev* e1000dev = GET_RAWDEV(e1000ndev);
	int index = e1000dev->regs[E1000_TDT];
	
	if(!e1000dev->tx_ring[index].status & E1000_TXD_STAT_DD) {
		printf("[e1000_transmit] still in progress\n");
		return -1;
	}
	
	// free mbuf
	struct mbuf *prev_mbuf = e1000dev->tx_mbuf[index == 0 ? TX_RING_SIZE-1: index-1];
	if (prev_mbuf != 0) {
		mbuffree(prev_mbuf);
		e1000dev->tx_mbuf[index == 0 ? TX_RING_SIZE-1: index-1] = 0;
	}

	e1000dev->tx_ring[index].addr = (uint64_t) m->head;
	e1000dev->tx_ring[index].length = (uint16_t) m->len;
	e1000dev->tx_ring[index].cmd = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;
	e1000dev->tx_ring[index].status = 0;

	e1000dev->tx_mbuf[index] = m;

	e1000dev->regs[E1000_TDT] = (index + 1) % TX_RING_SIZE;

	return 0;
}

static void e1000_recv(void) {
	struct e1000_dev* e1000dev = GET_RAWDEV(e1000ndev);
	// init
	int index = (e1000dev->regs[E1000_RDT]+1) % RX_RING_SIZE;
	if (e1000dev->rx_ring[index].status & E1000_RXD_STAT_DD) {
		struct mbuf *m = e1000dev->rx_mbuf[index];
		uint16_t len = e1000dev->rx_ring[index].length;
		e1000dev->rx_ring[index].status ^= E1000_RXD_STAT_DD;

		m->raddr = 0;
		m->next = 0;
		m->head = m->buf;
		m->len = 0;
		mbufput(m, len);

		e1000dev->regs[E1000_RDT] = index;
		index = (index+1) % RX_RING_SIZE;
		eth_recv(m);
	}
}

static void e1000_intr() {
	struct e1000_dev* e1000dev = GET_RAWDEV(e1000ndev);
	e1000dev->regs[E1000_ICR]; // clear pending interrupts
	e1000ndev->recv();
}

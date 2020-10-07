#pragma once

#include "ethernet.h"

#define TX_RING_SIZE 16
#define RX_RING_SIZE 16

struct netdev {
	const char*     name;
	struct spinlock lock;
	uint8_t         macaddr[ETH_ADDR_LEN];
	uint32_t*       regs;
};

#define NETDEV_ALIGNMENT 32;
#define NETDEV_ALIGN(a) (sizeof(a) + ALIGN(sizeof(struct netdev), NETDEV_ALIGNMENT))

#pragma once

#include "net/ethernet.h"
#include "net/mbuf.h"

#define TX_RING_SIZE 16
#define RX_RING_SIZE 16

struct netdev {
	const char*     name;
	uint8_t         macaddr[ETH_ADDR_LEN];
	int             (*transmit)(struct mbuf *m);
	void            (*recv)();
	void            (*intr)();
};

#define NETDEV_ALIGNMENT 32
#define NETDEV_ALIGN(a) (sizeof(a) + ALIGN(sizeof(struct netdev), NETDEV_ALIGNMENT-1))
#define GET_RAWDEV(a) (((void*)(a) + ALIGN(sizeof(struct netdev), NETDEV_ALIGNMENT-1)))

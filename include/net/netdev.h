#pragma once

#include "ethernet.h"

#define TX_RING_SIZE 16
#define RX_RING_SIZE 16

struct netdev {
	const char*     name;
	uint8_t         macaddr[ETH_ADDR_LEN];
};

#define NETDEV_ALIGNMENT 32
#define NETDEV_ALIGN(a) (sizeof(a) + ALIGN(sizeof(struct netdev), NETDEV_ALIGNMENT))
#define GET_RAWDEV(a) (((void*)(a) + ALIGN(sizeof(struct netdev), NETDEV_ALIGNMENT)))

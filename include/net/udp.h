#pragma once

#include "net/ethernet.h"
#include "net/ipv4.h"

// a UDP packet header (comes after an IP header).
struct udp {
	uint16_t sport; // source port
	uint16_t dport; // destination port
	uint16_t ulen;  // length, including udp header, not including IP header
	uint16_t sum;   // checksum
};

#define UDP_MAX_DATA (ETH_MAX_SIZE - sizeof(struct eth) - sizeof(struct ipv4) - sizeof(struct udp))

void udp_send(struct mbuf *, uint32_t, uint16_t, uint16_t);
void udp_recv(struct mbuf *, uint16_t, struct ipv4 *);

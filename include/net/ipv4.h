#pragma once

#include "net/mbuf.h"

// an IP packet header (comes after an Ethernet header).
struct ipv4 {
	uint8_t  ip_vhl; // version << 4 | header length >> 2
	uint8_t  ip_tos; // type of service
	uint16_t ip_len; // total length
	uint16_t ip_id;  // identification
	uint16_t ip_off; // fragment offset field
	uint8_t  ip_ttl; // time to live
	uint8_t  ip_p;   // protocol
	uint16_t ip_sum; // checksum
	uint32_t ip_src, ip_dst;
};

#define IPPROTO_ICMP 1  // Control message protocol
#define IPPROTO_TCP  6  // Transmission control protocol
#define IPPROTO_UDP  17 // User datagram protocol

#define IP_GET_FLG(off) (off & 0xe000)
#define IP_GET_OFF(off) (off & 0x1fff)

#define MAKE_IP_ADDR(a, b, c, d)           \
	(((uint32_t)a << 24) | ((uint32_t)b << 16) | \
	 ((uint32_t)c << 8) | (uint32_t)d)

void ip_send(struct mbuf *, uint8_t proto, uint32_t dip);
void ip_recv(struct mbuf *);

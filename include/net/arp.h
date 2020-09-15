#pragma once
// an ARP packet (comes after an Ethernet header).
struct arp {
	uint16_t hrd; // format of hardware address
	uint16_t pro; // format of protocol address
	uint8_t  hln; // length of hardware address
	uint8_t  pln; // length of protocol address
	uint16_t op;  // operation

	char   sha[ETH_ADDR_LEN]; // sender hardware address
	uint32_t sip;              // sender IP address
	char   tha[ETH_ADDR_LEN]; // target hardware address
	uint32_t tip;              // target IP address
} __attribute__((packed));

#define ARP_HRD_ETHER 1 // Ethernet

enum {
	ARP_OP_REQUEST = 1, // requests hw addr given protocol addr
	ARP_OP_REPLY = 2,   // replies a hw addr given protocol addr
};

int arp_send(uint16_t, uint8_t [], uint32_t);
void arp_recv(struct mbuf *);

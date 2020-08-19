#pragma once

#define ETH_ADDR_LEN 6

// an Ethernet packet header (start of the packet).
struct eth {
  uint8_t  dhost[ETH_ADDR_LEN];
  uint8_t  shost[ETH_ADDR_LEN];
  uint16_t type;
} __attribute__((packed));

#define ETH_TYPE_IP  0x0800 // Internet protocol
#define ETH_TYPE_ARP 0x0806 // Address resolution protocol

#define ETH_MAX_SIZE 1518

void eth_send(struct mbuf *, uint16_t, uint32_t dip);
void eth_recv(struct mbuf *);

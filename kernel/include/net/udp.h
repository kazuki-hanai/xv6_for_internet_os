#pragma once

#include "net/ipv4.h"

// a UDP packet header (comes after an IP header).
struct udp {
  uint16 sport; // source port
  uint16 dport; // destination port
  uint16 ulen;  // length, including udp header, not including IP header
  uint16 sum;   // checksum
};

void udp_send(struct mbuf *, uint32, uint16, uint16);
void udp_recv(struct mbuf *, uint16, struct ipv4 *);
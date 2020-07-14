#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "net/byteorder.h"
#include "net/mbuf.h"
#include "net/ethernet.h"
#include "net/arptable.h"
#include "net/arp.h"
#include "net/netutil.h"

extern struct mbufq arp_q;

uint8 local_mac[ETHADDR_LEN] = { 0x52, 0x54, 0x00, 0x12, 0x34, 0x56 };
uint8 broadcast_mac[ETHADDR_LEN] = { 0xFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF };

// sends an ethernet packet
void
net_tx_eth(struct mbuf *m, uint16 ethtype, uint32 dip)
{
  struct eth *ethhdr;

  uint8 dhost[ETHADDR_LEN] = {0, 0, 0, 0, 0, 0};
  if (arptable_get_mac(dip, (uint8 *)dhost) == -1) {
    if (ethtype != ETHTYPE_ARP) {
      net_tx_arp(ARP_OP_REQUEST, broadcast_mac, dip);
      mbufq_pushtail(&arp_q, m);
      return;
    } else {
      memmove(dhost, broadcast_mac, ETHADDR_LEN);
    }
  }
  ethhdr = mbufpushhdr(m, *ethhdr);
  ethhdr->type = htons(ethtype);
  memmove(ethhdr->shost, local_mac, ETHADDR_LEN);
  memmove(ethhdr->dhost, dhost, ETHADDR_LEN);

  e1000_transmit(m);
}

// called by e1000 driver's interrupt handler to deliver a packet to the
// networking stack
void net_rx(struct mbuf *m)
{
  struct eth *ethhdr;
  uint16 type;

  ethhdr = mbufpullhdr(m, *ethhdr);
  if (!ethhdr) {
    mbuffree(m);
    return;
  }

  type = ntohs(ethhdr->type);
  if (type == ETHTYPE_IP)
    net_rx_ip(m);
  else if (type == ETHTYPE_ARP)
    net_rx_arp(m);
  else
    mbuffree(m);
}

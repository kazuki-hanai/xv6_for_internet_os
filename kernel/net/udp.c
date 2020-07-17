#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "net/byteorder.h"
#include "net/mbuf.h"
#include "net/netutil.h"
#include "net/ipv4.h"
#include "net/udp.h"
#include "net/sock_cb.h"
#include "sys/sysnet.h"
#include "defs.h"

extern struct sock_cb_entry udp_scb_table[SOCK_CB_LEN];

// sends a UDP packet
void
udp_send(struct mbuf *m, uint32 dip,
           uint16 sport, uint16 dport)
{
  struct udp *udphdr;

  // put the UDP header
  udphdr = mbufpushhdr(m, *udphdr);
  udphdr->sport = htons(sport);
  udphdr->dport = htons(dport);
  udphdr->ulen = htons(m->len);
  udphdr->sum = 0; // zero means no checksum is provided

  // now on to the IP layer
  ip_send(m, IPPROTO_UDP, dip);
}

// receives a UDP packet
void
udp_recv(struct mbuf *m, uint16 len, struct ipv4 *iphdr)
{
  struct udp *udphdr;
  uint32 raddr;
  uint16 sport, dport;

  udphdr = mbufpullhdr(m, *udphdr);
  if (!udphdr)
    goto fail;

  // TODO: validate UDP checksum

  // validate lengths reported in headers
  if (ntohs(udphdr->ulen) != len)
    goto fail;
  len -= sizeof(*udphdr);
  if (len > m->len)
    goto fail;
  // minimum packet size could be larger than the payload
  mbuftrim(m, m->len - len);

  raddr = ntohl(iphdr->ip_src);
  sport = ntohs(udphdr->dport);
  dport = ntohs(udphdr->sport);

  struct sock_cb *scb = get_sock_cb(udp_scb_table, sport);;
  if (scb == 0) {
    goto fail;
  }
  if (scb->raddr == 0 && scb->dport == 0) {
    scb->raddr = raddr;
    scb->dport = dport;
  }
  // parse the necessary fields
  push_to_scb_rxq(scb, m);

  return;
fail:
  mbuffree(m);
}
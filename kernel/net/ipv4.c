#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "net/byteorder.h"
#include "net/mbuf.h"
#include "net/netutil.h"
#include "net/ethernet.h"
#include "net/ipv4.h"
#include "net/tcp.h"
#include "net/udp.h"
#include "defs.h"

uint32 local_ip = MAKE_IP_ADDR(192, 168, 22, 2); // qemu's idea of the guest IP

// sends an IP packet
void
ip_send(struct mbuf *m, uint8 proto, uint32 dip)
{
  struct ipv4 *iphdr;

  // push the IP header
  m->raddr = dip;
  iphdr = mbufpushhdr(m, *iphdr);
  memset(iphdr, 0, sizeof(*iphdr));
  iphdr->ip_vhl = (4 << 4) | (20 >> 2);
  iphdr->ip_p = proto;
  iphdr->ip_src = htonl(local_ip);
  iphdr->ip_dst = htonl(dip);
  iphdr->ip_len = htons(m->len);
  iphdr->ip_ttl = 100;
  iphdr->ip_sum = htons(cksum16((uint8 *)iphdr, sizeof(*iphdr), 0));

  // now on to the ethernet layer
  eth_send(m, ETH_TYPE_IP, dip);
}

// receives an IP packet
void
ip_recv(struct mbuf *m)
{
  struct ipv4 *iphdr;
  uint16 len;

  iphdr = mbufpullhdr(m, *iphdr);
  if (!iphdr)
	  goto fail;

  // check IP version and header len
  if (iphdr->ip_vhl != ((4 << 4) | (20 >> 2)))
    goto fail;
  // validate IP checksum
  if (cksum16((uint8 *)iphdr, sizeof(*iphdr), 0))
    goto fail;
  // can't support fragmented IP packets
  if (IP_GET_OFF(htons(iphdr->ip_off)) != 0)
    goto fail;
  // is the packet addressed to us?
  if (htonl(iphdr->ip_dst) != local_ip)
    goto fail;

  len = ntohs(iphdr->ip_len) - sizeof(*iphdr);

  if (iphdr->ip_p == IPPROTO_UDP) {
    udp_recv(m, len, iphdr);
  } else if (iphdr->ip_p == IPPROTO_TCP) {
    tcp_recv(m, len, iphdr);
  } else {
    goto fail;
  }
  return;

fail:
  mbuffree(m);
}

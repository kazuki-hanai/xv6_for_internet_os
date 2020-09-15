#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "net/byteorder.h"
#include "net/mbuf.h"
#include "net/netutil.h"
#include "net/ethernet.h"
#include "net/arp.h"
#include "net/arptable.h"

extern struct arp_tabel arptable;
extern uint8_t local_mac[];
extern uint32_t local_ip;

struct mbufq arp_q;

void arpinit() {
	mbufq_init(&arp_q);
	arptable_init();
}

// sends an ARP packet
int
arp_send(uint16_t op, uint8_t dmac[ETH_ADDR_LEN], uint32_t dip)
{
	struct mbuf *m;
	struct arp *arphdr;

	m = mbufalloc(MBUF_DEFAULT_HEADROOM);
	if (!m)
		return -1;

	// generic part of ARP header
	arphdr = mbufputhdr(m, *arphdr);
	arphdr->hrd = htons(ARP_HRD_ETHER);
	arphdr->pro = htons(ETH_TYPE_IP);
	arphdr->hln = ETH_ADDR_LEN;
	arphdr->pln = sizeof(uint32_t);
	arphdr->op = htons(op);

	// ethernet + IP part of ARP header
	memmove(arphdr->sha, local_mac, ETH_ADDR_LEN);
	arphdr->sip = htonl(local_ip);
	memmove(arphdr->tha, dmac, ETH_ADDR_LEN);
	arphdr->tip = htonl(dip);

	// header is ready, send the packet
	eth_send(m, ETH_TYPE_ARP, dip);
	return 0;
}

// receives an ARP packet
void
arp_recv(struct mbuf *m)
{
	struct arp *arphdr;
	uint8_t smac[ETH_ADDR_LEN];
	uint32_t sip, tip;

	arphdr = mbufpullhdr(m, *arphdr);
	if (!arphdr)
		goto done;

	// validate the ARP header
	if (ntohs(arphdr->hrd) != ARP_HRD_ETHER ||
			ntohs(arphdr->pro) != ETH_TYPE_IP ||
			arphdr->hln != ETH_ADDR_LEN ||
			arphdr->pln != sizeof(uint32_t)) {
		goto done;
	}

	
	memmove(smac, arphdr->sha, ETH_ADDR_LEN); // sender's ethernet address
	tip = ntohl(arphdr->tip); // target IP address
	sip = ntohl(arphdr->sip); // sender's IP address (qemu's slirp)
	arptable_add(sip, smac);

	// check queue
	struct mbuf *prev = 0;
	struct mbuf *now = arp_q.head;
	while(now != 0) {
		if (now->raddr == sip) {
			eth_send(now, ETH_TYPE_IP, now->raddr);
			if (prev != 0) {
				prev->next = now->next;
				now = now->next;
			} else {
				arp_q.head = now->next;
				now = now->next;
			}
			continue;
		}
		prev = now;
		now = now->next;
	}

	// only requests are supported so far
	// check if our IP was solicited
	if (ntohs(arphdr->op) != ARP_OP_REQUEST || tip != local_ip)
		goto done;

	// handle the ARP request
	arp_send(ARP_OP_REPLY, smac, sip);

done:
	mbuffree(m);
}

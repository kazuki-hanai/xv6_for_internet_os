#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "defs.h"
#include "spinlock.h"
#include "net/ethernet.h"
#include "net/arptable.h"
#include "net/sock_cb.h"

extern struct arp_cache_entry arptable[ARP_DEFUALT_ENTRY_NUM];
struct arp_cache* get_arp_cache(uint32_t ip);
extern struct mbufq tx_queue;
void nic_mock_recv(struct mbuf *m);
uint64_t sys_sockconnect_core(struct sock_cb *scb, uint32_t raddr, uint16_t dport);
uint64_t sys_socklisten_core(struct sock_cb *scb, uint16_t sport);

void arp_table_test();
void arp_packet_test();

void arp_test() {
	printf("\t[arp test] start...\n");
	arp_table_test();
	arp_packet_test();
	printf("\t[arp test] done!\n");
}

void arp_packet_test() {
	printf("\t\t[arp_packet test] start...\n");
	struct mbuf* create_packet(char *bytes, int len) {
		struct mbuf *buf = mbufalloc(ETH_MAX_SIZE);
		mbufput(buf, len);
		memmove((void *)buf->head, (void *)bytes, len);
		return buf;
	}

	char arp_request_packet_bytes[] = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x5e, 0x02,
		0x03, 0x04, 0x05, 0x06, 0x08, 0x06, 0x00, 0x01,
		0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0x5e, 0x02,
		0x03, 0x04, 0x05, 0x06, 0xc0, 0xa8, 0x03, 0x02,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xa8,
		0x16, 0x02
	};
	char arp_reply_packet_bytes[] = {
		0x5e, 0x02, 0x03, 0x04, 0x05, 0x06, 0x52, 0x54,
		0x00, 0x12, 0x34, 0x56, 0x08, 0x06, 0x00, 0x01,
		0x08, 0x00, 0x06, 0x04, 0x00, 0x02, 0x52, 0x54,
		0x00, 0x12, 0x34, 0x56, 0xc0, 0xa8, 0x16, 0x02,
		0x5e, 0x02, 0x03, 0x04, 0x05, 0x06, 0xc0, 0xa8,
		0x03, 0x02
	};

	// arp request
	struct mbuf *arp_request = create_packet(arp_request_packet_bytes, sizeof(arp_request_packet_bytes));
	nic_mock_recv(arp_request);

	// arp reply
	struct mbuf *arp_reply = mbufq_pophead(&tx_queue);
	if (memcmp(arp_reply->head, arp_reply_packet_bytes, arp_reply->len) != 0) {
		panic("not match arp_reply");
	}
	mbuffree(arp_reply);
	printf("\t\t[arp_packet test] done!\n");
}

void arp_table_test() {
	printf("\t\t[arp table test] start...\n");
	uint32_t ip1 = 0x00000001;
	uint32_t ip2 = 0x00000002;
	uint32_t ip3 = ip1 + ARP_DEFUALT_ENTRY_NUM;

	uint8_t mac1[ETH_ADDR_LEN];
	memset(mac1, 0, ETH_ADDR_LEN);
	mac1[ETH_ADDR_LEN-1] = 0x1;

	uint8_t mac2[ETH_ADDR_LEN];
	memset(mac2, 0, ETH_ADDR_LEN);
	mac2[ETH_ADDR_LEN-1] = 0x2;

	uint8_t mac3[ETH_ADDR_LEN];
	memset(mac3, 0, ETH_ADDR_LEN);
	mac3[ETH_ADDR_LEN-1] = 0x3;

	arptable_add(ip1, mac1);
	arptable_add(ip2, mac2);
	arptable_add(ip3, mac3);

	struct arp_cache *arpcache;

	arpcache = get_arp_cache(ip1);
	if (arpcache->ip != ip1) {
		panic("ip1 does not exist\n");
	}
	if (memcmp(mac1, arpcache->mac, ETH_ADDR_LEN) != 0) {
		panic("mac1 does not exist\n");
	}
	if (arpcache->prev != 0) {
		panic("ip1 cache has prev\n");
	}
	if (arpcache->next == 0) {
		panic("ip1 cache must have ip2 cache\n");
	}
	
	arpcache = get_arp_cache(ip2);
	if (arpcache->ip != ip2) {
		panic("ip2 does not exist\n");
	}
	if (memcmp(mac2, arpcache->mac, ETH_ADDR_LEN) != 0) {
		panic("mac2 does not exist\n");
	}
	
	arpcache = get_arp_cache(ip3);
	if (arpcache->ip != ip3) {
		panic("ip3 does not exist\n");
	}
	if (memcmp(mac3, arpcache->mac, ETH_ADDR_LEN) != 0) {
		panic("mac3 does not exist\n");
	}
	if (arpcache->prev->ip != ip1) {
		panic("Linked List broken!(because ip1 does not exist\n");
	}

	arptable_del(ip1);
	arpcache = get_arp_cache(ip3);
	if (arpcache->ip != ip3) {
		panic("ip3 does not exist\n");
	}
	if (arpcache->prev != 0) {
		panic("ip3 must be head");
	}
	if (arpcache->next != 0) {
		panic("ip3 must not have next");
	}
	printf("\t\t[arp table test] done!\n");
}

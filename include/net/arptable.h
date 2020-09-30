#pragma once

#define ARP_DEFUALT_ENTRY_NUM 32

struct arp_cache {
	int resolved;
	uint32_t ip;
	uint8_t mac[ETH_ADDR_LEN];
	struct arp_cache *prev;
	struct arp_cache *next;
};

struct arp_cache_entry {
	struct spinlock lock;
	struct arp_cache *head;
};

void arptable_init();
void arptable_add(uint32_t, uint8_t*);
int arptable_get_mac(uint32_t, uint8_t*);
void arptable_del(uint32_t);

#pragma once

#define ARP_DEFUALT_ENTRY_NUM 32

struct arp_cache {
  int resolved;
  uint32 ip;
  uint8 mac[ETHADDR_LEN];
  struct arp_cache *prev;
  struct arp_cache *next;
};

struct arp_cache_entry {
  struct spinlock lock;
  struct arp_cache *head;
};

void arptable_init();
void arptable_add(uint32, uint8*);
void arptable_get_mac(uint32, uint8*);
void arptable_del(uint32);
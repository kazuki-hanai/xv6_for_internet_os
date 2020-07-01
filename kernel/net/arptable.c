#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "net/byteorder.h"
#include "net/mbuf.h"
#include "net/ethernet.h"
#include "net/netutil.h"
#include "net/arptable.h"

struct arp_cache_entry arptable[ARP_DEFUALT_ENTRY_NUM];

void arp_cache_free(struct arp_cache *arpcache) {
  struct arp_cache_entry *entry = &arptable[arpcache->ip % ARP_DEFUALT_ENTRY_NUM];
  acquire(&entry->lock);
  if (arpcache->prev != 0)
    arpcache->prev->next = arpcache->next;
  if (arpcache->next != 0)
    arpcache->next->prev = arpcache->prev;
  else
    entry->head = arpcache->next;
  bd_free(arpcache);
  release(&entry->lock);
}

struct arp_cache* get_arp_cache(uint32 ip) {
  struct arp_cache_entry *entry = &arptable[ip % ARP_DEFUALT_ENTRY_NUM];
  struct arp_cache *arpcache;
  struct arp_cache *prev;

  acquire(&entry->lock);
  arpcache = entry->head;
  prev = 0;
  while (arpcache != 0) {
    if (arpcache->ip == ip)
      break;
    prev = arpcache;
    arpcache = arpcache->next;
  }

  // new cache
  if (arpcache == 0) {
    arpcache = bd_alloc(sizeof(struct arp_cache));
    if (arpcache == 0)
      panic("arp alloc failed!\n");
    arpcache->resolved = 0;
    arpcache->ip = ip;
    if (prev != 0)
      prev->next = arpcache;
    arpcache->prev = prev;
    arpcache->next = 0;
    memset(arpcache->mac, 0, ETHADDR_LEN);
  // already exists
  } else if (
    arpcache != 0 &&
    arpcache->ip == ip
  ) { }

  if (entry->head == 0)
    entry->head = arpcache;

  release(&entry->lock);
  return arpcache;
}

void arptable_init() {
  for (int i = 0; i < ARP_DEFUALT_ENTRY_NUM; i++) {
    arptable[i].head = 0;
    initlock(&arptable[i].lock, "arp table lock");
  }
}

void arptable_add(uint32 ip, uint8 *mac) {
  struct arp_cache *arpcache;

  arpcache = get_arp_cache(ip);
  arpcache->resolved = 1;
  memmove(arpcache->mac, mac, ETHADDR_LEN);
}

void arptable_get_mac(uint32 ip, uint8 *mac) {
  struct arp_cache *arpcache = get_arp_cache(ip);
  uint8 broadcast_mac[ETHADDR_LEN] = { 0xFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF };
  if (arpcache->resolved)
    memmove(mac, arpcache->mac, ETHADDR_LEN);
  else
    memmove(mac, broadcast_mac, ETHADDR_LEN);
}

void arptable_del(uint32 ip) {
  struct arp_cache *arpcache;

  arpcache = get_arp_cache(ip);
  arp_cache_free(arpcache);
}

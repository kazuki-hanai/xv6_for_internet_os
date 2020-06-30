#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "defs.h"
#include "spinlock.h"
#include "net/ethernet.h"
#include "net/arptable.h"

extern struct arp_cache_entry arptable[ARP_DEFUALT_ENTRY_NUM];
struct arp_cache* get_arp_cache(uint32 ip);

void arp_table_test();

void arp_test() {
  printf("\t[arp test] start...\n");
  arp_table_test();
  printf("\t[arp test] done!\n");
}

void arp_table_test() {
  printf("\t\t[arp table test] start...\n");
  uint32 ip1 = 0x00000001;
  uint32 ip2 = 0x00000002;
  uint32 ip3 = ip1 + ARP_DEFUALT_ENTRY_NUM;

  uint8 mac1[ETHADDR_LEN];
  memset(mac1, 0, ETHADDR_LEN);
  mac1[ETHADDR_LEN-1] = 0x1;

  uint8 mac2[ETHADDR_LEN];
  memset(mac2, 0, ETHADDR_LEN);
  mac2[ETHADDR_LEN-1] = 0x2;

  uint8 mac3[ETHADDR_LEN];
  memset(mac3, 0, ETHADDR_LEN);
  mac3[ETHADDR_LEN-1] = 0x3;

  arptable_add(ip1, mac1);
  arptable_add(ip2, mac2);
  arptable_add(ip3, mac3);

  struct arp_cache *arpcache;

  arpcache = get_arp_cache(ip1);
  if (arpcache->ip != ip1) {
    panic("ip1 does not exist\n");
  }
  if (memcmp(mac1, arpcache->mac, ETHADDR_LEN) != 0) {
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
  if (memcmp(mac2, arpcache->mac, ETHADDR_LEN) != 0) {
    panic("mac2 does not exist\n");
  }
  
  arpcache = get_arp_cache(ip3);
  if (arpcache->ip != ip3) {
    panic("ip3 does not exist\n");
  }
  if (memcmp(mac3, arpcache->mac, ETHADDR_LEN) != 0) {
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
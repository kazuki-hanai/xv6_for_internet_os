#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "net/mbuf.h"
#include "net/tcp.h"
#include "net/sock_cb.h"

extern struct sock_cb_entry tcp_scb_table[SOCK_CB_LEN];
extern struct sock_cb_entry udp_scb_table[SOCK_CB_LEN];

void tcp_scb_table_test();
void udp_scb_table_test();

void sock_cb_test() {
  printf("\t[sock_cb test] start...\n");
  tcp_scb_table_test();
  udp_scb_table_test();
  printf("\t[sock_cb test] done...\n");
}

void tcp_scb_table_test() {
  printf("\t\t[tcp_scb_table test] start...\n");
  uint32_t raddr1 = 0x01;
  uint32_t raddr2 = 0x02;
  uint32_t raddr3 = 0x03;
  uint16_t sport1 = 0x01;
  uint16_t sport2 = 0x02;
  uint16_t sport3 = 0x01 + SOCK_CB_LEN;
  uint16_t dport1 = 0x01;
  uint16_t dport2 = 0x02;
  uint16_t dport3 = 0x03;

  if (sport1 % SOCK_CB_LEN != sport3 % SOCK_CB_LEN)
    panic("two sports hash should equal!\n");
  
  struct sock_cb *scb;
  scb = alloc_sock_cb(0, raddr1, sport1, dport1, SOCK_TCP);
  add_sock_cb(scb);
  scb = alloc_sock_cb(0, raddr2, sport2, dport2, SOCK_TCP);
  add_sock_cb(scb);
  scb = alloc_sock_cb(0, raddr3, sport3, dport3, SOCK_TCP);
  add_sock_cb(scb);

  struct sock_cb *scb1 = get_sock_cb(tcp_scb_table, sport1);
  struct sock_cb *scb2 = get_sock_cb(tcp_scb_table, sport2);
  struct sock_cb *scb3 = get_sock_cb(tcp_scb_table, sport3);

  if (scb1 == 0) {
    panic("scb1 is null\n");
  }
  if (scb2 == 0) {
    panic("scb2 is null\n");
  }
  if (scb3 == 0) {
    panic("scb3 is null\n");
  }

  if (scb1->raddr != raddr1) {
    panic("not match raddr1\n");
  }
  if (scb1->sport != sport1) {
    panic("not match sport1\n");
  }
  if (scb1->dport != dport1) {
    panic("not match dport1\n");
  }
  if (scb1->prev != 0) {
    panic("not match scb1's prev\n");
  }
  if (scb1->next != scb3) {
    panic("not match scb1's next\n");
  }

  if (scb2->raddr != raddr2) {
    panic("not match raddr2\n");
  }
  if (scb2->sport != sport2) {
    panic("not match sport2\n");
  }
  if (scb2->dport != dport2) {
    panic("not match dport2\n");
  }
  if (scb2->prev != 0) {
    panic("not match scb2's prev\n");
  }
  if (scb2->next != 0) {
    panic("not match scb2's next\n");
  }

  if (scb3->raddr != raddr3) {
    panic("not match raddr3\n");
  }
  if (scb3->sport != sport3) {
    panic("not match sport3\n");
  }
  if (scb3->dport != dport3) {
    panic("not match dport3\n");
  }
  if (scb3->prev->sport != sport1) {
    panic("not match scb3's prev sport\n");
  }
  if (scb3->prev->next != scb3) {
    panic("not match scb3's prev next\n");
  }
  if (scb3->next != 0) {
    panic("not match scb3's next\n");
  }

  free_sock_cb(scb1);
  if (scb3->prev != 0) {
    panic("not match scb3's prev\n");
  }
  if (scb3->next != 0) {
    panic("not match scb3's next\n");
  }
  printf("\t\t[tcp_scb_table_test] done!\n");
}

void udp_scb_table_test() {
  printf("\t\t[udp_scb_table test] start...\n");
  uint32_t raddr1 = 0x01;
  uint32_t raddr2 = 0x02;
  uint32_t raddr3 = 0x03;
  uint16_t sport1 = 0x01;
  uint16_t sport2 = 0x02;
  uint16_t sport3 = 0x01 + SOCK_CB_LEN;
  uint16_t dport1 = 0x01;
  uint16_t dport2 = 0x02;
  uint16_t dport3 = 0x03;

  if (sport1 % SOCK_CB_LEN != sport3 % SOCK_CB_LEN)
    panic("two sports hash should equal!\n");
  
  struct sock_cb *scb;
  scb = alloc_sock_cb(0, raddr1, sport1, dport1, SOCK_UDP);
  add_sock_cb(scb);
  scb = alloc_sock_cb(0, raddr2, sport2, dport2, SOCK_UDP);
  add_sock_cb(scb);
  scb = alloc_sock_cb(0, raddr3, sport3, dport3, SOCK_UDP);
  add_sock_cb(scb);

  struct sock_cb *scb1 = get_sock_cb(udp_scb_table, sport1);
  struct sock_cb *scb2 = get_sock_cb(udp_scb_table, sport2);
  struct sock_cb *scb3 = get_sock_cb(udp_scb_table, sport3);

  if (scb1 == 0) {
    panic("scb1 is null\n");
  }
  if (scb2 == 0) {
    panic("scb2 is null\n");
  }
  if (scb3 == 0) {
    panic("scb3 is null\n");
  }

  if (scb1->raddr != raddr1) {
    panic("not match raddr1\n");
  }
  if (scb1->sport != sport1) {
    panic("not match sport1\n");
  }
  if (scb1->dport != dport1) {
    panic("not match dport1\n");
  }
  if (scb1->prev != 0) {
    panic("not match scb1's prev\n");
  }
  if (scb1->next != scb3) {
    panic("not match scb1's next\n");
  }

  if (scb2->raddr != raddr2) {
    panic("not match raddr2\n");
  }
  if (scb2->sport != sport2) {
    panic("not match sport2\n");
  }
  if (scb2->dport != dport2) {
    panic("not match dport2\n");
  }
  if (scb2->prev != 0) {
    panic("not match scb2's prev\n");
  }
  if (scb2->next != 0) {
    panic("not match scb2's next\n");
  }

  if (scb3->raddr != raddr3) {
    panic("not match raddr3\n");
  }
  if (scb3->sport != sport3) {
    panic("not match sport3\n");
  }
  if (scb3->dport != dport3) {
    panic("not match dport3\n");
  }
  if (scb3->prev->sport != sport1) {
    panic("not match scb3's prev sport\n");
  }
  if (scb3->prev->next != scb3) {
    panic("not match scb3's prev next\n");
  }
  if (scb3->next != 0) {
    panic("not match scb3's next\n");
  }

  free_sock_cb(scb1);
  if (scb3->prev != 0) {
    panic("not match scb3's prev\n");
  }
  if (scb3->next != 0) {
    panic("not match scb3's next\n");
  }
  printf("\t\t[udp_scb_table_test] done!\n");
}

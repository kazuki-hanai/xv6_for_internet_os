#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "net/mbuf.h"
#include "net/tcp.h"
#include "net/sock_cb.h"

extern struct sock_cb_entry scb_table[SOCK_CB_LEN];
void scb_hash_table_test();

void tcp_test() {
  printf("\t[tcp test] start...\n");
  scb_hash_table_test();
  printf("\t[tcp test] done...\n");
}

void scb_hash_table_test() {
  printf("\t\t[tcp_hash_table_test] start...\n");
  // uint32 raddr1 = 0x01;
  // uint32 raddr2 = 0x02;
  // uint32 raddr3 = 0x03;
  // uint16 sport1 = 0x01;
  // uint16 sport2 = 0x02;
  // uint16 sport3 = 0x03;
  // uint16 dport1 = 0x01;
  // uint16 dport2 = 0x02;
  // uint16 dport3 = 0x03 + TCP_CB_LEN - 4;

  // if ((raddr1 + (sport1 << 16) + dport1) % SOCK_CB_LEN !=(raddr3 + (sport3 << 16) + dport3) % SOCK_CB_LEN)
  //   panic("hash not equal!\n");

  // struct sock_cb *scb;
  
  // scb = get_tcb(raddr1, sport1, dport1);
  // if (scb->raddr != raddr1) {
  //   panic("not match raddr1\n");
  // }
  // if (scb->sport != sport1) {
  //   panic("not match sport1\n");
  // }
  // if (scb->dport != dport1) {
  //   panic("not match dport1\n");
  // }
  // if (scb->prev != 0) {
  //   panic("not match scb1's prev\n");
  // }
  // if (scb->next != 0) {
  //   panic("not match scb1's next\n");
  // }
  // scb = get_sock_cb(raddr2, sport2, dport2);
  // if (scb->raddr != raddr2) {
  //   panic("not match raddr2\n");
  // }
  // if (scb->sport != sport2) {
  //   panic("not match sport2\n");
  // }
  // if (scb->dport != dport2) {
  //   panic("not match dport2\n");
  // }
  // if (scb->prev != 0) {
  //   panic("not match scb2's prev\n");
  // }
  // if (scb->next != 0) {
  //   panic("not match scb2's next\n");
  // }

  // scb = get_sock_cb(raddr3, sport3, dport3);
  // if (scb->raddr != raddr3) {
  //   panic("not match raddr3\n");
  // }
  // if (scb->sport != sport3) {
  //   panic("not match sport3\n");
  // }
  // if (scb->dport != dport3) {
  //   panic("not match dport3\n");
  // }
  // if (scb->prev->sport != sport1) {
  //   panic("not match scb3's prev sport\n");
  // }
  // if (scb->prev->next != tcb) {
  //   panic("not match scb3's prev next\n");
  // }
  // if (scb->next != 0) {
  //   panic("not match scb3's next\n");
  // }

  // scb = get_sock_cb(raddr1, sport1, dport1);
  // free_sock_cb(scb);
  // scb = get_scb(raddr3, sport3, dport3);
  // if (scb->raddr != raddr3 || tcb->sport != sport3 || tcb->dport != dport3) {
  //   panic("not match scb3\n");
  // }
  // if (scb->prev != 0) {
  //   panic("not match scb3's prev\n");
  // }
  // if (scb->next != 0) {
  //   panic("not match scb3's next\n");
  // }
  printf("\t\t[tcp_hash_table_test] done!\n");
}
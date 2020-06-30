#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "net/mbuf.h"
#include "net/tcp.h"

struct tcp_cb* get_tcb(uint32 raddr, uint16 sport, uint16 dport);
void free_tcp_cb(struct tcp_cb *tcb);
extern struct tcp_cb_entry tcb_table[TCP_CB_LEN];
void tcb_hash_table_test();

void tcp_test() {
  printf("\t[tcp test] start...\n");
  tcb_hash_table_test();
  printf("\t[tcp test] done...\n");
}

void tcb_hash_table_test() {
  printf("\t\t[tcp_hash_table_test] start...\n");
  uint32 raddr1 = 0x01;
  uint32 raddr2 = 0x02;
  uint32 raddr3 = 0x03;
  uint16 sport1 = 0x01;
  uint16 sport2 = 0x02;
  uint16 sport3 = 0x03;
  uint16 dport1 = 0x01;
  uint16 dport2 = 0x02;
  uint16 dport3 = 0x03 + TCP_CB_LEN - 4;

  if ((raddr1 + (sport1 << 16) + dport1) % TCP_CB_LEN !=(raddr3 + (sport3 << 16) + dport3) % TCP_CB_LEN)
    panic("hash not equal!\n");

  struct tcp_cb *tcb;
  
  tcb = get_tcb(raddr1, sport1, dport1);
  if (tcb->raddr != raddr1) {
    panic("not match raddr1\n");
  }
  if (tcb->sport != sport1) {
    panic("not match sport1\n");
  }
  if (tcb->dport != dport1) {
    panic("not match dport1\n");
  }
  if (tcb->prev != 0) {
    panic("not match tcb1's prev\n");
  }
  if (tcb->next != 0) {
    panic("not match tcb1's next\n");
  }
  tcb = get_tcb(raddr2, sport2, dport2);
  if (tcb->raddr != raddr2) {
    panic("not match raddr2\n");
  }
  if (tcb->sport != sport2) {
    panic("not match sport2\n");
  }
  if (tcb->dport != dport2) {
    panic("not match dport2\n");
  }
  if (tcb->prev != 0) {
    panic("not match tcb2's prev\n");
  }
  if (tcb->next != 0) {
    panic("not match tcb2's next\n");
  }

  tcb = get_tcb(raddr3, sport3, dport3);
  if (tcb->raddr != raddr3) {
    panic("not match raddr3\n");
  }
  if (tcb->sport != sport3) {
    panic("not match sport3\n");
  }
  if (tcb->dport != dport3) {
    panic("not match dport3\n");
  }
  if (tcb->prev->sport != sport1) {
    panic("not match tcb3's prev sport\n");
  }
  if (tcb->prev->next != tcb) {
    panic("not match tcb3's prev next\n");
  }
  if (tcb->next != 0) {
    panic("not match tcb3's next\n");
  }

  tcb = get_tcb(raddr1, sport1, dport1);
  free_tcp_cb(tcb);
  tcb = get_tcb(raddr3, sport3, dport3);
  if (tcb->raddr != raddr3 || tcb->sport != sport3 || tcb->dport != dport3) {
    panic("not match tcb3\n");
  }
  if (tcb->prev != 0) {
    panic("not match tcb3's prev\n");
  }
  if (tcb->next != 0) {
    panic("not match tcb3's next\n");
  }
  printf("\t\t[tcp_hash_table_test] done!\n");
}
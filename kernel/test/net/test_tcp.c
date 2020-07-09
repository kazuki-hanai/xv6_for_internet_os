#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "net/mbuf.h"
#include "net/ethernet.h"
#include "net/tcp.h"
#include "net/sock_cb.h"

extern struct mbufq tx_buf;
void nic_mock_recv(struct mbuf *m);
uint64 sys_sockconnect_core(struct sock_cb *scb, uint32 raddr, uint16 dport);
uint64 sys_socklisten_core(struct sock_cb *scb, uint16 sport);

void listen_handshake_test();

void tcp_test() {
  printf("\t[tcp test] start...\n");
  listen_handshake_test();
  printf("\t[tcp test] done...\n");
}

void listen_handshake_test() {
  printf("\t\t[listen_handshake test] start...\n");

  uint16 sport = 2000;
  struct sock_cb *scb = init_sock_cb(0, 0, 0, SOCK_TCP);
  if (sys_socklisten_core(scb, sport) < 0) {
    panic("sys_socklisten_core failed!");
  }
  
  char syn_packet_raw[] = {
    0x52, 0x54, 0x00, 0x12, 0x34, 0x56, 0x3a, 0x61,
    0x0a, 0xc0, 0x49, 0xea, 0x08, 0x00, 0x45, 0x00,
    0x00, 0x3c, 0x3f, 0xf9, 0x40, 0x00, 0x40, 0x06,
    0x2c, 0x6a, 0xc0, 0xa8, 0x37, 0x06, 0xc0, 0xa8,
    0x16, 0x02, 0x80, 0x1a, 0x07, 0xd0, 0x6a, 0x1e,
    0x6c, 0x3e, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02,
    0xfa, 0xf0, 0x6d, 0x52, 0x00, 0x00, 0x02, 0x04,
    0x05, 0xb4, 0x04, 0x02, 0x08, 0x0a, 0x32, 0xf2,
    0x80, 0x2a, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03,
    0x03, 0x07
  };
  struct mbuf *syn_buf = mbufalloc(ETH_MAX_SIZE);
  mbufput(syn_buf, sizeof(syn_packet_raw));
  memmove((void *)syn_buf->head, (void *)syn_packet_raw, sizeof(syn_packet_raw));
  nic_mock_recv(syn_buf);

  printf("\t\t[listen_handshake test] done...\n");
}

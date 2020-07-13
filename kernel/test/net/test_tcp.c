#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "net/mbuf.h"
#include "net/ethernet.h"
#include "net/tcp.h"
#include "net/sock_cb.h"

extern struct mbufq tx_queue;
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
  struct mbuf* create_packet(char *bytes, int len) {
    struct mbuf *buf = mbufalloc(ETH_MAX_SIZE);
    mbufput(buf, len);
    memmove((void *)buf->head, (void *)bytes, len);
    return buf;
  }
  char syn_packet_bytes[] = {
    0x52, 0x54, 0x00, 0x12, 0x34, 0x56, 0x92, 0xb9,
    0xc9, 0xa4, 0x40, 0xff, 0x08, 0x00, 0x45, 0x00,
    0x00, 0x3c, 0xf7, 0x38, 0x40, 0x00, 0x40, 0x06,
    0xa9, 0x2e, 0xc0, 0xa8, 0x03, 0x02, 0xc0, 0xa8,
    0x16, 0x02, 0x8f, 0xc4, 0x07, 0xd0, 0xa6, 0x12,
    0xc6, 0xb6, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02,
    0xfa, 0xf0, 0xb4, 0xc3, 0x00, 0x00, 0x02, 0x04,
    0x05, 0xb4, 0x04, 0x02, 0x08, 0x0a, 0x05, 0x85,
    0xf3, 0xf1, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03,
    0x03, 0x07
  };
  char syn_ack_packet_bytes[] = {
    0x92, 0xb9, 0xc9, 0xa4, 0x40, 0xff, 0x52, 0x54,
    0x00, 0x12, 0x34, 0x56, 0x08, 0x00, 0x45, 0x00,
    0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x64, 0x06,
    0xbc, 0x7b, 0xc0, 0xa8, 0x16, 0x02, 0xc0, 0xa8,
    0x03, 0x02, 0x07, 0xd0, 0x8f, 0xc4, 0x00, 0x00,
    0x00, 0x00, 0xde, 0xd0, 0x3c, 0x0c, 0x50, 0x12,
    0x10, 0x00, 0x53, 0x0c, 0x00, 0x00
  };
  char ack_packet_bytes[] = {
    0x92, 0xb9, 0xc9, 0xa4, 0x40, 0xff, 0x52, 0x54,
    0x00, 0x12, 0x34, 0x56, 0x08, 0x00, 0x45, 0x00,
    0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x64, 0x06,
    0xbc, 0x7b, 0xc0, 0xa8, 0x16, 0x02, 0xc0, 0xa8,
    0x03, 0x02, 0x07, 0xd0, 0x8f, 0xc4, 0xde, 0xd0,
    0x3c, 0x0c, 0xde, 0xd0, 0x3c, 0x0c, 0x50, 0x10,
    0x10, 0x00, 0x38, 0x31, 0x00, 0x00
  };

  // syn packet
  struct mbuf *syn = create_packet(syn_packet_bytes, sizeof(syn_packet_bytes));
  nic_mock_recv(syn);

  // syn ack packet
  struct mbuf *syn_ack = mbufq_pophead(&tx_queue);
  for (int i = 0; i < syn_ack->len; i++) {
    printf("%x ", syn_ack->head[i]);
  }
  if (memcmp(syn_ack->head, syn_ack_packet_bytes, syn_ack->len) != 0) {
    panic("not match syn_ack");
  }
  mbuffree(syn_ack);

  // ack
  struct mbuf *ack = create_packet(ack_packet_bytes, sizeof(ack_packet_bytes));
  nic_mock_recv(ack);

  // no packet transmit
  if (mbufq_pophead(&tx_queue) != 0) {
    panic("why transmit a packet??");
  }

  printf("\t\t[listen_handshake test] done...\n");
}

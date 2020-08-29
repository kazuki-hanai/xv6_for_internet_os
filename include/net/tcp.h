#pragma once

#include "net/ethernet.h"
#include "net/sock_cb.h"
#include "net/ipv4.h"

#define TCP_COLLISION_NUM 11
#define TCP_MOD 2 << 31
#define TCP_DEFAULT_WINDOW 65535

#define TCP_FLG_FIN 0x01
#define TCP_FLG_SYN 0x02
#define TCP_FLG_RST 0x04
#define TCP_FLG_PSH 0x08
#define TCP_FLG_ACK 0x10
#define TCP_FLG_URG 0x20

#define TCP_OP_RETRANS -4
#define TCP_OP_CLOSE_SCB -3
#define TCP_OP_SND_RST -2
#define TCP_OP_ERR -1
#define TCP_OP_OK 0
#define TCP_OP_SND_ACK 1
#define TCP_OP_TXT_OK 2
#define TCP_OP_FREE_MBUF 3

#define TCP_MIN_PORT 25000
#define TCP_MAX_DATA (ETH_MAX_SIZE - sizeof(struct eth) - sizeof(struct ipv4) - sizeof(struct tcp))


#define TCP_HDR_LEN(hdr) (((hdr)->off >> 4) << 2)
#define TCP_DATA_LEN(hdr, len) ((len)-TCP_HDR_LEN(hdr))
#define TCP_FLG_ISSET(x, y) (((x)&0x3f) & (y))


struct tcp {
  uint16_t sport;
  uint16_t dport;
  uint32_t seq;
  uint32_t ack;
// little endian only
// use #define to check endian if you want to support big endian
  uint8_t off;
  uint8_t flg;
  uint16_t wnd;
  uint16_t sum;
  uint16_t urg;
};

int tcp_listen(struct sock_cb *);
int tcp_connect(struct sock_cb *);
int tcp_send(struct sock_cb *, struct mbuf *m, uint8_t);
int tcp_close(struct sock_cb *);
void tcp_recv(struct mbuf *, uint16_t, struct ipv4 *);

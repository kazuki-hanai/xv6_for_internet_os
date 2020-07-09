#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "net/byteorder.h"
#include "net/mbuf.h"
#include "net/netutil.h"
#include "net/ethernet.h"
#include "net/ipv4.h"
#include "net/tcp.h"
#include "net/sock_cb.h"

extern struct sock_cb_entry tcp_scb_table[SOCK_CB_LEN];

void tcpinit() {
  
}

static uint16 tcp_checksum(uint32 ip_src, uint32 ip_dst, uint8 ip_p, struct tcp *tcphdr, uint16 len) {
  uint32 pseudo = 0;

  pseudo += ntohs((ip_src >> 16) & 0xffff);
  pseudo += ntohs(ip_src & 0xffff);
  pseudo += ntohs((ip_dst >> 16) & 0xffff);
  pseudo += ntohs(ip_dst & 0xffff);
  pseudo += (uint16)ip_p;
  pseudo += len;
  return cksum16((uint16 *)tcphdr, len, pseudo);
}

int tcp_listen(struct sock_cb *scb) {
  if (scb->socktype != SOCK_TCP) {
    printf("not tcp socket!\n");
    return -1;
  }
  scb->state = SOCK_CB_LISTEN;
  // TODO SOCK_CB_LISTEN STATE
  // -> chenge the connection from passive to active
  // "error: connection already exists"

  return 0;
}

int tcp_connect(struct sock_cb *scb) {
  if (scb->socktype != SOCK_TCP) {
    printf("not tcp socket!\n");
    return -1;
  }
  struct mbuf *m = mbufalloc(ETH_MAX_SIZE);
  scb->rcv.wnd = 2048;
  net_tx_tcp(scb, m, TCP_FLG_SYN, 0);
  scb->state = SOCK_CB_SYN_SENT;
  scb->snd.nxt_seq = scb->snd.init_seq + 1;

  // TODO SOCK_CB_LISTEN STATE
  // -> chenge the connection from passive to active
  // "error: connection already exists"

  return 0;
}

// https://tools.ietf.org/html/rfc793#page-56
int tcp_send(struct sock_cb *scb) {
  enum sock_cb_state state = scb->state;

  if (TCP_FLG_ISSET(state, SOCK_CB_CLOSED)) {
    // "error: connection illegal for this process"
    return -1;
  }

  if (TCP_FLG_ISSET(state, SOCK_CB_LISTEN)) {
    if (scb->raddr) {
      uint32 seq = 0; // initial send sequence number;
      // uint8 flag = TCP_FLG_SYN;

      acquire(&scb->lock);
      scb->snd.unack = seq;
      scb->snd.nxt_seq = seq+1;
      scb->state = SOCK_CB_SYN_SENT;
      // TODO The urgent bit if requested in the command must be sent with the data segments sent
      // as a result of this command.

      release(&scb->lock);

      return 0;
    } else {
      // "error: foreign socket unspecified";
      return -1;
    }
  }

  // TODO
  // SOCK_CB_SYN_SENT
  // SOCK_CB_SYN_RCVD
  // SOCK_CB_ESTAB
  // SOCK_CB_CLOSE_WAIT
  // SOCK_CB_FIN_WAIT_1
  // SOCK_CB_FIN_WAIT_2
  // SOCK_CB_CLOSING
  // SOCK_CB_LAST_ACK
  // SOCK_CB_TIME_WAIT
  return -1;
}

int tcp_recv(struct sock_cb *scb, struct mbuf *m, struct tcp *tcphdr) {
  enum sock_cb_state state = scb->state;
  if (TCP_FLG_ISSET(state, SOCK_CB_CLOSED)) {
    // "error: connection illegal for this process"
    return -1;
  }

  // SOCK_CB_LISTEN
  // SOCK_CB_SYN_SENT
  // SOCK_CB_SYN_RCVD
  if (
    TCP_FLG_ISSET(state, SOCK_CB_LISTEN) ||
    TCP_FLG_ISSET(state, SOCK_CB_SYN_SENT) ||
    TCP_FLG_ISSET(state, SOCK_CB_SYN_RCVD)
  ) {
    // queue mbuf
  }
  
  // SOCK_CB_ESTAB
  // SOCK_CB_FIN_WAIT_1
  // SOCK_CB_FIN_WAIT_2
  //
  // SOCK_CB_CLOSE_WAIT
  //
  // SOCK_CB_CLOSING
  // SOCK_CB_LAST_ACK
  // SOCK_CB_TIME_WAIT
  return -1;
}

int tcp_close(struct sock_cb *scb) {
  return -1;
}

int tcp_abort() {
  return -1;
}

void net_tx_tcp(struct sock_cb *scb, struct mbuf *m, uint8 flg, uint16 payload_len) {
  extern uint32 local_ip;
  struct tcp *tcphdr;

  tcphdr = mbufpushhdr(m, *tcphdr);
  tcphdr->sport = htons(scb->sport);
  tcphdr->dport = htons(scb->dport);
  tcphdr->seq = htonl(scb->snd.nxt_seq);
  tcphdr->ack = htonl(scb->rcv.nxt_seq);
  tcphdr->off = (sizeof(struct tcp) >> 2) << 4;
  tcphdr->flg = flg;
  printf("flg: %d\n", flg);
  tcphdr->wnd = htons(scb->rcv.wnd);
  tcphdr->urg = 0;
  tcphdr->sum = 0;
  tcphdr->sum = htons(tcp_checksum(htonl(local_ip), htonl(scb->raddr), IPPROTO_TCP, tcphdr, payload_len + sizeof(struct tcp)));
  net_tx_ip(m, IPPROTO_TCP, scb->raddr);
}

// segment arrives
void net_rx_tcp(struct mbuf *m, uint16 len, struct ipv4 *iphdr) {
  struct tcp *tcphdr;
  uint16 dport, sport;
  uint32 raddr;
  struct sock_cb *scb = 0;

  tcphdr = mbufpullhdr(m, *tcphdr);
  if (!tcphdr)
    goto fail;

  uint16 sum = tcp_checksum(iphdr->ip_src, iphdr->ip_dst, iphdr->ip_p, tcphdr, len);
  if (sum != 0) {
    printf("[bad tcp] checksum doesn't match\n");
    goto fail;
  }

  raddr = ntohl(iphdr->ip_src);
  dport = ntohs(tcphdr->sport);
  sport = ntohs(tcphdr->dport);

  scb = get_sock_cb(tcp_scb_table, sport);

  if (scb == 0)
    goto fail;

  acquire(&scb->lock);
  if (scb == 0) {
    goto fail;
  }

  uint8 flg = tcphdr->flg;
  uint32 ack = ntohl(tcphdr->ack);
  uint32 seq = ntohl(tcphdr->seq);

  // TODO check seq & ack

  if (scb->state == SOCK_CB_CLOSED) {
    // TODO if a incoming packet does not contain a RST, send a RST packet.
    goto fail;
  } else if (scb->state == SOCK_CB_LISTEN) {
    // if received SYN, send SYN,ACK
    if (TCP_FLG_ISSET(flg, TCP_FLG_RST)) {
      goto fail;
    }
    if (TCP_FLG_ISSET(flg, TCP_FLG_ACK)) {
      struct mbuf *m = mbufalloc(ETH_MAX_SIZE);
      net_tx_tcp(scb, m, TCP_FLG_RST | TCP_FLG_ACK, 0);
      goto fail;
      // TODO send RST
    }
    if (TCP_FLG_ISSET(flg, TCP_FLG_SYN)) {
      // TODO check security
      // TODO If the SEG.PRC is greater than the TCB.PRC
      scb->dport = dport;
      scb->raddr = raddr;

      // TODO window
      scb->rcv.wnd = 2048;
      scb->rcv.init_seq = ntohl(tcphdr->seq);
      scb->rcv.nxt_seq = scb->rcv.init_seq + 1;
      scb->snd.init_seq = 0;
      scb->snd.nxt_seq = 0;
      struct mbuf *m = mbufalloc(ETH_MAX_SIZE);
      net_tx_tcp(scb, m, TCP_FLG_SYN | TCP_FLG_ACK, 0);
      scb->snd.nxt_seq = scb->rcv.init_seq + 1;
      scb->snd.unack = scb->rcv.init_seq;
      // TODO timeout
      scb->state = SOCK_CB_SYN_RCVD;
    } else {
      goto fail;
    }
  // SYN/ACK received
  } else if (scb->state == SOCK_CB_SYN_SENT) {
    if (TCP_FLG_ISSET(flg, TCP_FLG_ACK)) {
      if (ack <= scb->snd.init_seq || ack > scb->snd.nxt_seq) {
        struct mbuf *m = mbufalloc(ETH_MAX_SIZE);
        net_tx_tcp(scb, m, TCP_FLG_RST | TCP_FLG_ACK, 0);
        goto fail;
      }
    }
    if (TCP_FLG_ISSET(flg, TCP_FLG_RST)) {
      // TODO free sock
      goto fail;
    }
    // TODO check the security and precedence

    if (TCP_FLG_ISSET(flg, TCP_FLG_SYN | TCP_FLG_ACK)) {
      scb->rcv.nxt_seq = seq + 1;
      scb->rcv.init_seq = seq;
      scb->snd.unack = ack;

      if (scb->snd.unack > scb->snd.init_seq) {
        scb->state = SOCK_CB_ESTAB;
        struct mbuf *m = mbufalloc(ETH_MAX_SIZE);
        net_tx_tcp(scb, m, TCP_FLG_ACK, 0);
      } else {
        scb->state = SOCK_CB_SYN_RCVD;
        struct mbuf *m = mbufalloc(ETH_MAX_SIZE);
        net_tx_tcp(scb, m, TCP_FLG_SYN | TCP_FLG_ACK, 0);
      }
    } else {
      goto fail;
    }
  } else {

  }

  // TODO URG process

  release(&scb->lock);
  return;

fail:
  if(scb != 0)
    release(&scb->lock);
  mbuffree(m);
}

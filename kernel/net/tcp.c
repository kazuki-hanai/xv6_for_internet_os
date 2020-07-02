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

extern struct sock_cb_entry scb_table[TCP_CB_LEN];

void tcpinit() {
  
}

static uint16 tcp_checksum(struct ipv4 *iphdr , struct tcp *tcphdr, uint16 len) {
  uint32 pseudo = 0;

  pseudo += ntohs((iphdr->ip_src >> 16) & 0xffff);
  pseudo += ntohs(iphdr->ip_src & 0xffff);
  pseudo += ntohs((iphdr->ip_dst >> 16) & 0xffff);
  pseudo += ntohs(iphdr->ip_dst & 0xffff);
  pseudo += (uint16)iphdr->ip_p;
  pseudo += len;
  return cksum16((uint16 *)tcphdr, len, pseudo);
}

struct sock_cb *tcp_open(uint32 raddr, uint16 sport, uint16 dport, int sock_type) {
  struct sock_cb *scb;
  scb = get_sock_cb(raddr, sport, dport, sock_type);
  if (sock_type == SOCK_TCP) {
    struct mbuf *m = mbufalloc(ETH_MAX_SIZE);
    net_tx_tcp(scb, m, TCP_FLG_SYN);
    scb->state = SYN_SENT;
  } else if (sock_type == SOCK_TCP_LISTEN) {
    scb->state = LISTEN;
  } else {
    panic("[tcp_open] You tried udp, didn't you?");
  }
  // TODO LISTEN STATE
  // -> chenge the connection from passive to active
  // "error: connection already exists"

  return scb;
}

// https://tools.ietf.org/html/rfc793#page-56
int tcp_send(struct sock_cb *scb) {
  enum sock_cb_state state = scb->state;

  if (TCP_FLG_ISSET(state, CLOSED)) {
    // "error: connection illegal for this process"
    return -1;
  }

  if (TCP_FLG_ISSET(state, LISTEN)) {
    if (scb->raddr) {
      uint32 seq = 0; // initial send sequence number;
      // uint8 flag = TCP_FLG_SYN;

      acquire(&scb->lock);
      scb->snd.unack = seq;
      scb->snd.nxt_seq = seq+1;
      scb->state = SYN_SENT;
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
  // SYN_SENT
  // SYN_RCVD
  // ESTAB
  // CLOSE_WAIT
  // FIN_WAIT_1
  // FIN_WAIT_2
  // CLOSING
  // LAST_ACK
  // TIME_WAIT
  return -1;
}

int tcp_recv(struct sock_cb *scb, struct mbuf *m, struct tcp *tcphdr) {
  enum sock_cb_state state = scb->state;
  if (TCP_FLG_ISSET(state, CLOSED)) {
    // "error: connection illegal for this process"
    return -1;
  }

  // LISTEN
  // SYN_SENT
  // SYN_RCVD
  if (
    TCP_FLG_ISSET(state, LISTEN) ||
    TCP_FLG_ISSET(state, SYN_SENT) ||
    TCP_FLG_ISSET(state, SYN_RCVD)
  ) {
    // queue mbuf
  }
  
  // ESTAB
  // FIN_WAIT_1
  // FIN_WAIT_2
  //
  // CLOSE_WAIT
  //
  // CLOSING
  // LAST_ACK
  // TIME_WAIT
  return -1;
}

int tcp_close(struct sock_cb *scb) {
  return -1;
}

int tcp_abort() {
  return -1;
}

void net_tx_tcp(struct sock_cb *scb, struct mbuf *m, uint8 flg) {
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
  // tcphdr->sum = tcp_checksum(iphdr, tcphdr, len);
  tcphdr->sum = 0;
  tcphdr->urg = 0;

  printf("ip: %x\n", scb->raddr);
  net_tx_ip(m, IPPROTO_TCP, scb->raddr);
}


// void tcp_rx_core(struct sock_cb *scb, struct mbuf *m, struct tcphdr *tcphdr, struct ipv4 *iphdr) {
// }

// segment arrives
void net_rx_tcp(struct mbuf *m, uint16 len, struct ipv4 *iphdr) {
  struct tcp *tcphdr;
  uint16 dport, sport;
  uint32 raddr;
  struct sock_cb *scb = 0;

  tcphdr = mbufpullhdr(m, *tcphdr);
  if (!tcphdr)
    goto fail;

  uint16 sum = tcp_checksum(iphdr, tcphdr, len);
  if (sum != 0) {
    printf("[bad tcp] checksum doesn't match\n");
    goto fail;
  }

  raddr = ntohl(iphdr->ip_src);
  dport = ntohs(tcphdr->dport);
  sport = ntohs(tcphdr->sport);

  scb = get_sock_cb(raddr, dport, sport, SOCK_UNKNOWN);

  printf("ok?\n");
  acquire(&scb->lock);
  if (scb == 0) {
    goto fail;
  }
  printf("ok!\n");

  uint8 flg = tcphdr->flg;

  // TODO check seq & ack

  if (scb->state == CLOSED) {
    goto fail;
  } else if (scb->state == LISTEN) {
    // If received SYN, send SYN,ACK
    if (TCP_FLG_ISSET(flg, TCP_FLG_RST)) {
      goto fail;
    } else if (TCP_FLG_ISSET(flg, TCP_FLG_ACK)) {
      goto fail;
      // TODO send RST
    } else if (TCP_FLG_ISSET(flg, TCP_FLG_SYN)) {
      // TODO check security
      // TODO If the SEG.PRC is greater than the TCB.PRC
      // TODO sport
      scb->dport = sport;

      // TODO window
      scb->rcv.wnd = 65535;
      scb->rcv.init_seq = ntohl(tcphdr->seq);
      scb->rcv.nxt_seq = scb->rcv.init_seq + 1;
      scb->snd.init_seq = 0;
      scb->snd.nxt_seq = 0;
      struct mbuf *m = mbufalloc(ETH_MAX_SIZE);
      net_tx_tcp(scb, m, TCP_FLG_SYN | TCP_FLG_ACK);
      scb->snd.nxt_seq = scb->rcv.init_seq + 1;
      scb->snd.unack = scb->rcv.init_seq;
      // TODO timeout
      scb->state = SYN_RCVD;
    } else {
      goto fail;
    }
  } else if (scb->state == SYN_SENT) {
  } else if (scb->state == SYN_RCVD) {
  } else if (scb->state == ESTAB) {
  } else if (scb->state == FIN_WAIT_1) {
  } else if (scb->state == FIN_WAIT_2) {
  } else if (scb->state == CLOSING) {
  } else if (scb->state == TIME_WAIT) {
  } else if (scb->state == CLOSE_WAIT) {
  } else if (scb->state == LAST_ACK) {
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

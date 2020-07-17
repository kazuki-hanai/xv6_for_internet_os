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

static uint16 tcp_checksum(uint32, uint32, uint8, struct tcp *, uint16);
static void tcp_send_core(struct mbuf *, uint32, uint16, uint16, uint32, uint32, uint32, uint8, uint16);
static int is_seq_valid(uint32, uint32, uint32, uint16);
static int check_rst_flg(struct sock_cb *, uint8);
static int check_ack(struct sock_cb *, uint8, uint32, uint32, uint32);

struct rx_tcp_context {
  struct sock_cb *scb;
  struct mbuf *m;
  struct tcp *tcphdr;
  uint32 raddr;
  uint16 sport;
  uint16 dport;
  uint16 len;
};

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
  uint32 res = cksum16((uint8 *)tcphdr, len, pseudo);
  return res;
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
  scb->rcv.wnd = SOCK_CB_DEFAULT_WND_SIZE;
  tcp_send_core(m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_SYN, 0);
  scb->state = SOCK_CB_SYN_SENT;
  scb->snd.nxt_seq = scb->snd.init_seq + 1;

  // TODO SOCK_CB_LISTEN STATE
  // -> chenge the connection from passive to active
  // "error: connection already exists"

  return 0;
}

// https://tools.ietf.org/html/rfc793#page-56
int tcp_send(struct sock_cb *scb, struct mbuf *m, uint8 flg) {
  if (scb == 0)
    return -1;
  if (m == 0)
    return -1;

  if (scb->state == SOCK_CB_CLOSED) {
    // "error: connection illegal for this process"
    return -1;
  }

  switch(scb->state) {
  case SOCK_CB_LISTEN:
    // TODO
    // if (scb->raddr) {
    //   uint32 seq = 0; // initial send sequence number;
    //   // uint8 flag = TCP_FLG_SYN;

    //   scb->snd.unack = seq;
    //   scb->snd.nxt_seq = seq+1;
    //   scb->state = SOCK_CB_SYN_SENT;
    //   // TODO The urgent bit if requested in the command must be sent with the data segments sent
    //   // as a result of this command.

    //   return 0;
    // } else {
    //   // "error: foreign socket unspecified";
    //   return -1;
    // }
    return -1;
    break;
  case SOCK_CB_SYN_SENT:
  case SOCK_CB_SYN_RCVD:
    push_to_scb_txq(scb, m, scb->snd.nxt_seq, flg, m->len);
    break;
  case SOCK_CB_ESTAB:
  case SOCK_CB_CLOSE_WAIT:
    // TODO windows processing
    // send packet
    push_to_scb_txq(scb, m, scb->snd.nxt_seq, flg, m->len);
    struct mbuf *send_m = pop_from_scb_txq(scb);
    while(send_m) {
      // TODO buffer processing
      int sndnxt = m->params.tcp.sndnxt;
      int flg = m->params.tcp.flg;
      int len = m->params.tcp.datalen;
      tcp_send_core(send_m, scb->raddr, scb->sport, scb->dport, sndnxt, scb->rcv.nxt_seq, scb->rcv.wnd, flg, len);
      scb->snd.nxt_seq += len;
      send_m = pop_from_scb_txq(scb);
    }
    break;
  case SOCK_CB_FIN_WAIT_1:
  case SOCK_CB_FIN_WAIT_2:
  case SOCK_CB_CLOSING:
  case SOCK_CB_LAST_ACK:
  case SOCK_CB_TIME_WAIT:
    // "error:  connection closing" and do not service request.
    return -1;
    break;
  default:
    return -1;
    break;
  }

  return 0;
}

int tcp_close(struct sock_cb *scb) {
  struct mbuf *m = 0;
  switch(scb->state) {
  case SOCK_CB_CLOSED:
    // "error:  connection illegal for this process".
    return -1;
    break;
  case SOCK_CB_LISTEN:
    // "error:  closing"
    return -1;
    break;
  case SOCK_CB_SYN_SENT:
    // "error closing"
    return -1;
    break;
  case SOCK_CB_SYN_RCVD:
    m = mbufalloc(ETH_MAX_SIZE);
    if (!mbufq_empty(&scb->txq)) {
      tcp_send_core(m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_FIN, m->len);
      scb->state = SOCK_CB_FIN_WAIT_1;
    } else {
      push_to_scb_txq(scb, m, scb->snd.nxt_seq, TCP_FLG_FIN, m->len);
    }
    break;
  case SOCK_CB_ESTAB:
    m = mbufalloc(ETH_MAX_SIZE);
    push_to_scb_txq(scb, m, scb->snd.nxt_seq, TCP_FLG_FIN, m->len);
    // TODO send and entering FIN_WAIT_1
    break;
  case SOCK_CB_FIN_WAIT_1:
  case SOCK_CB_FIN_WAIT_2:
    // "error: connection closing"
    return -1;
    break;
  case SOCK_CB_CLOSE_WAIT:
    m = mbufalloc(ETH_MAX_SIZE);
    push_to_scb_txq(scb, m, scb->snd.nxt_seq, TCP_FLG_FIN, m->len);
    // TODO send and entering CLOSING
    break;
  default:
    return -1;
  }
  return 0;
}

int tcp_abort() {
  return -1;
}

static void tcp_send_core(
  struct mbuf *m,
  uint32 raddr,
  uint16 sport,
  uint16 dport,
  uint32 sndnxt,
  uint32 rcvnxt,
  uint32 rcvwnd,
  uint8 flg,
  uint16 payload_len
) {
  extern uint32 local_ip;
  struct tcp *tcphdr;

  tcphdr = mbufpushhdr(m, *tcphdr);
  tcphdr->sport = htons(sport);
  tcphdr->dport = htons(dport);
  tcphdr->seq = htonl(sndnxt);
  if (TCP_FLG_ISSET(flg, TCP_FLG_ACK))
    tcphdr->ack = htonl(rcvnxt);
  else
    tcphdr->ack = 0;
  tcphdr->off = (sizeof(struct tcp) >> 2) << 4;
  tcphdr->flg = flg;
  tcphdr->wnd = htons(rcvwnd);
  tcphdr->urg = 0;
  tcphdr->sum = 0;
  tcphdr->sum = htons(tcp_checksum(htonl(local_ip), htonl(raddr), IPPROTO_TCP, tcphdr, payload_len + sizeof(struct tcp)));
  ip_send(m, IPPROTO_TCP, raddr);
}

static int is_seq_valid(uint32 rcvwnd, uint32 rcvnxt, uint32 segseq, uint16 seglen) {
  if (rcvwnd == 0) {
    return (seglen == 0 && segseq == rcvnxt) && (seglen == 0);
  } else {
    if (seglen == 0) {
      return rcvnxt <= segseq && segseq < (rcvnxt+rcvwnd);
    } else {
      return  (rcvnxt <= segseq && segseq < (rcvnxt+rcvwnd)) ||
              (rcvnxt <= (segseq+seglen-1) && (segseq+seglen-1) < (rcvnxt+rcvwnd));
    }
  }
}

static int tcp_recv_listen(struct rx_tcp_context *ctxt) {
  struct tcp *tcphdr = ctxt->tcphdr;
  struct sock_cb *scb = ctxt->scb;
  uint32 raddr = ctxt->raddr;
  uint16 sport = ctxt->sport;
  uint16 dport = ctxt->dport;
  uint32 ack = ntohl(tcphdr->ack);
  uint32 seq = ntohl(tcphdr->seq);
  uint8 flg = tcphdr->flg;

  // if received SYN, send SYN,ACK
  if (TCP_FLG_ISSET(flg, TCP_FLG_RST)) {
    return -1;
  }
  if (TCP_FLG_ISSET(flg, TCP_FLG_ACK)) {
    struct mbuf *ack_m = mbufalloc(ETH_MAX_SIZE);
    tcp_send_core(ack_m, raddr, sport, dport, ack, 0, scb->rcv.wnd, TCP_FLG_RST | TCP_FLG_ACK, 0);
    return -1;
  }
  if (TCP_FLG_ISSET(flg, TCP_FLG_SYN)) {
    // TODO check security
    // TODO If the SEG.PRC is greater than the TCB.PRC
    scb->dport = dport;
    scb->raddr = raddr;

    // TODO window
    scb->rcv.wnd = SOCK_CB_DEFAULT_WND_SIZE;
    scb->rcv.init_seq = seq;
    scb->rcv.nxt_seq = seq + 1;
    scb->snd.init_seq = 0;
    scb->snd.nxt_seq = 1;
    scb->snd.unack = scb->snd.init_seq;
    struct mbuf *syn_ack_m = mbufalloc(ETH_MAX_SIZE);
    tcp_send_core(syn_ack_m, scb->raddr, scb->sport, scb->dport, scb->snd.init_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_SYN | TCP_FLG_ACK, 0);
    // TODO timeout
    scb->state = SOCK_CB_SYN_RCVD;
  } else {
    return -1;
  }
  return 0;
}

static int tcp_recv_syn_sent(struct rx_tcp_context *ctxt) {
  struct tcp *tcphdr = ctxt->tcphdr;
  struct sock_cb *scb = ctxt->scb;
  uint32 ack = ntohl(tcphdr->ack);
  uint32 seq = ntohl(tcphdr->seq);
  uint8 flg = tcphdr->flg;

  // SYN/ACK received
  if (TCP_FLG_ISSET(flg, TCP_FLG_ACK)) {
    if (ack <= scb->snd.init_seq || ack > scb->snd.nxt_seq) {
      struct mbuf *rst_ack_m = mbufalloc(ETH_MAX_SIZE);
      tcp_send_core(rst_ack_m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_RST | TCP_FLG_ACK, 0);
      scb->snd.nxt_seq = scb->snd.init_seq + 1;
      return -1;
    }
  }
  if (TCP_FLG_ISSET(flg, TCP_FLG_RST)) {
    // TODO free sock
    return -1;
  }
  // TODO check the security and precedence

  if (TCP_FLG_ISSET(flg, TCP_FLG_SYN) && TCP_FLG_ISSET(flg, TCP_FLG_ACK)) {
    scb->rcv.nxt_seq = seq + 1;
    scb->rcv.init_seq = seq;
    scb->snd.unack = ack;

    if (scb->snd.unack > scb->snd.init_seq) {
      scb->state = SOCK_CB_ESTAB;
      struct mbuf *ack_m = mbufalloc(ETH_MAX_SIZE);
      tcp_send_core(ack_m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_ACK, 0);
    } else {
      scb->state = SOCK_CB_SYN_RCVD;
      struct mbuf *syn_ack_m = mbufalloc(ETH_MAX_SIZE);
      tcp_send_core(syn_ack_m, scb->raddr, scb->sport, scb->dport, scb->snd.init_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_SYN | TCP_FLG_ACK, 0);
    }
  } else {
    return -1;
  }
  return 0;
}

static int check_rst_flg(struct sock_cb *scb, uint8 flg) {
  if (TCP_FLG_ISSET(flg, TCP_FLG_RST)) {
    switch(scb->state) {
      case SOCK_CB_SYN_RCVD:
        scb->state = SOCK_CB_LISTEN;
        return -1;
        break;
      case SOCK_CB_ESTAB:
      case SOCK_CB_FIN_WAIT_1:
      case SOCK_CB_FIN_WAIT_2:
      case SOCK_CB_CLOSE_WAIT:
        // TODO
        // segment queues should be flushed
        // close scb
        return -1;
        break;
      case SOCK_CB_CLOSING:
      case SOCK_CB_LAST_ACK:
      case SOCK_CB_TIME_WAIT:
        // TODO
        // close scb
        break;
      default:
        break;
    }
  }
  return 0;
}

static int check_ack(struct sock_cb *scb, uint8 flg, uint32 ack, uint32 seq, uint32 sndwnd) {
  if (!TCP_FLG_ISSET(flg, TCP_FLG_ACK)) {
    return -1;
  }
  switch(scb->state) {
    case SOCK_CB_SYN_RCVD:
      if (scb->snd.unack <= ack && ack <= scb->snd.nxt_seq) {
        scb->state = SOCK_CB_ESTAB;
      } else {
        struct mbuf *rst_m = mbufalloc(ETH_MAX_SIZE);
        tcp_send_core(rst_m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_RST, 0);
        return -1;
      }
      break;
    case SOCK_CB_ESTAB:
    case SOCK_CB_FIN_WAIT_1:
    case SOCK_CB_FIN_WAIT_2:
    case SOCK_CB_CLOSE_WAIT:
    case SOCK_CB_CLOSING:
    case SOCK_CB_LAST_ACK:
      if (scb->snd.unack < ack && ack <= scb->snd.nxt_seq) {
        scb->snd.unack = ack;
        // TODO
        // Any segments on the retransmission queue which are thereby entirely
        // acknowledged are removed
        if (scb->snd.wl1 < seq || (scb->snd.wl1 == seq && scb->snd.wl2 <= ack)) {
          scb->snd.wnd = sndwnd;
          scb->snd.wl1 = seq;
          scb->snd.wl2 = ack;
        }
      } else if (ack < scb->snd.unack) {
        return -1;
      } else if (scb->snd.nxt_seq < ack) {
        // TODO
        // If the ACK acks something not yet sent then send an ACK, drop the segment
      }

      if (scb->state == SOCK_CB_FIN_WAIT_1) {
        scb->state = SOCK_CB_FIN_WAIT_2;
      }
      if (scb->state == SOCK_CB_FIN_WAIT_2) {
        // TODO check whether retransmission queue is empty
        // ??????????????????????
        scb->state = SOCK_CB_TIME_WAIT;
      }
      if (scb->state == SOCK_CB_CLOSING) {
        scb->state = SOCK_CB_TIME_WAIT;
      }
      break;
    case SOCK_CB_TIME_WAIT:
      // TODO
      // The only thing that can arrive in this state is a 
      // retransmission of the remote FIN. Acknowledge it, and restart
      // the 2 MSL timeout
      break;
    default:
      break;
  }
  return 0;
}

static int tcp_recv_core(struct rx_tcp_context *ctxt) {
  struct tcp *tcphdr = ctxt->tcphdr;
  struct sock_cb *scb = ctxt->scb;
  struct mbuf *m = ctxt->m;
  uint32 ack = ntohl(tcphdr->ack);
  uint32 seq = ntohl(tcphdr->seq);
  uint8 flg = tcphdr->flg;
  uint16 datalen = TCP_DATA_LEN(tcphdr, ctxt->len);

  // first check sequence number
  if (!is_seq_valid(scb->rcv.wnd, scb->rcv.nxt_seq, seq, datalen)) {
    // send ack(unless the RST bit is set, if so drop the segment and return)
    if (!TCP_FLG_ISSET(flg, TCP_FLG_RST)) {
      struct mbuf *rst_m = mbufalloc(ETH_MAX_SIZE);
      tcp_send_core(rst_m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_ACK, 0);
    }
    return -1;
  }

  // second check the RST bit
  if (check_rst_flg(scb, flg) == -1) {
    return -1;
  }
  
  // TODO third check security and precednece
  
  // fource, check the SYN bit
  if (
    scb->state == SOCK_CB_SYN_RCVD ||
    scb->state == SOCK_CB_ESTAB ||
    scb->state == SOCK_CB_FIN_WAIT_1 ||
    scb->state == SOCK_CB_FIN_WAIT_2 ||
    scb->state == SOCK_CB_CLOSE_WAIT ||
    scb->state == SOCK_CB_CLOSING ||
    scb->state == SOCK_CB_LAST_ACK ||
    scb->state == SOCK_CB_TIME_WAIT
  ) {
    // TODO
    // If the SYN is in the window it is an error, send reset, close scb
  }

  // fifth check the ACK field
  if (check_ack(scb, flg, ack, seq, ntohl(tcphdr->wnd)) == -1) {
    return -1;
  }
  
  // TODO sixth, check the URG bit

  // seventh, process the segment text
  // deliver segment text to user RECEIVE buffers
  switch(scb->state) {
    case SOCK_CB_ESTAB:
    case SOCK_CB_FIN_WAIT_1:
    case SOCK_CB_FIN_WAIT_2:
      if (datalen > 0 && scb->rcv.nxt_seq == seq) {
        scb->rcv.nxt_seq += datalen;
        push_to_scb_rxq(scb, m);
        struct mbuf *ack_m = mbufalloc(ETH_MAX_SIZE);
        tcp_send_core(ack_m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_ACK, 0);
      }
      break;
    case SOCK_CB_CLOSE_WAIT:
    case SOCK_CB_CLOSING:
    case SOCK_CB_LAST_ACK:
    case SOCK_CB_TIME_WAIT:
      // This should not occur, since a FIN has been received from the
      // remote side. Ignore the segment text.
      return -1;
      break;
    default:
      break;
  }

  // eighth, check the FIN bit
  if (TCP_FLG_ISSET(flg, TCP_FLG_FIN)) {
    switch(scb->state) {
      case SOCK_CB_CLOSED:
      case SOCK_CB_LISTEN:
      case SOCK_CB_SYN_SENT:
        return -1;
        break;
      // signal the user "connection closing"
      // return any pending RECEIVEs with above message
      // advance RCV.NXT over the FIN
      // send ack for the FIN
      // Note that FIN implies PUSH for any segment text not yet delivered to the user.
      case SOCK_CB_SYN_RCVD:
      case SOCK_CB_ESTAB:
        scb->state = SOCK_CB_CLOSE_WAIT;
        break;
      case SOCK_CB_FIN_WAIT_1:
        // TODO
        // If our FIN has been ACKed, then enter TIME_WAIT, start the time-wait timer
        // otherwise enter the CLOSING state;
        scb->state = SOCK_CB_TIME_WAIT;
        break;
      case SOCK_CB_FIN_WAIT_2:
        scb->state = SOCK_CB_TIME_WAIT;
        break;
      case SOCK_CB_CLOSE_WAIT:
      case SOCK_CB_CLOSING:
      case SOCK_CB_LAST_ACK:
      case SOCK_CB_TIME_WAIT:
        // remain in the CLOSE_WAIT
        // If state is TIME_WAIT, restart the 2 MSL time_wait timeout.
        break;
    }
  }
  return 0;
}

// segment arrives
void tcp_recv(struct mbuf *m, uint16 len, struct ipv4 *iphdr) {
  struct sock_cb *scb = 0;
  // pull header
  struct tcp *tcphdr = mbufpullhdr(m, *tcphdr);
  if (!tcphdr)
    goto fail;
  uint32 raddr = ntohl(iphdr->ip_src);
  uint16 sport = ntohs(tcphdr->dport);
  uint16 dport = ntohs(tcphdr->sport);

  // checksum
  uint16 sum = tcp_checksum(iphdr->ip_src, iphdr->ip_dst, iphdr->ip_p, tcphdr, len);
  if (sum) {
    printf("[bad tcp] checksum doesn't match, %d, len: %d\n", sum, len);
    goto fail;
  }

  // get scb
  scb = get_sock_cb(tcp_scb_table, sport);
  if (scb == 0)
    goto fail;

  // early return
  if (scb->state != SOCK_CB_LISTEN && scb->state != SOCK_CB_CLOSED && scb->dport != dport) {
    goto fail;
  }

  // create context
  struct rx_tcp_context ctxt;
  ctxt.scb = scb;
  ctxt.m = m;
  ctxt.tcphdr = tcphdr;
  ctxt.raddr = raddr;
  ctxt.sport = sport;
  ctxt.dport = dport;
  ctxt.len = len;

  // state processing
  switch(scb->state) {
    case SOCK_CB_CLOSED:
      // TODO if a incoming packet does not contain a RST, send a RST packet.
      goto fail;
      break;
    case SOCK_CB_LISTEN:
      if (tcp_recv_listen(&ctxt) == -1)
        goto fail;
      mbuffree(m);
      break;
    case SOCK_CB_SYN_SENT:
      if (tcp_recv_syn_sent(&ctxt) == -1)
        goto fail;
      mbuffree(m);
      break;
    case SOCK_CB_SYN_RCVD:
    case SOCK_CB_ESTAB:
    case SOCK_CB_FIN_WAIT_1:
    case SOCK_CB_FIN_WAIT_2:
    case SOCK_CB_CLOSE_WAIT:
    case SOCK_CB_CLOSING:
    case SOCK_CB_LAST_ACK:
    case SOCK_CB_TIME_WAIT:
      if (tcp_recv_core(&ctxt) == -1) {
        goto fail;
      }
      break;
  }

  // TODO URG process
  return;
fail:
  mbuffree(m);
  return;
}

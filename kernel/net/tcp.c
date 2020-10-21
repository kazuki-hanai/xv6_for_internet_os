#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "file.h"
#include "net/byteorder.h"
#include "net/mbuf.h"
#include "net/netutil.h"
#include "net/ethernet.h"
#include "net/ipv4.h"
#include "net/tcp.h"
#include "net/sock_cb.h"

extern struct sock_cb_entry tcp_scb_table[SOCK_CB_LEN];

static uint16_t tcp_checksum(uint32_t, uint32_t, uint8_t, struct tcp *, uint16_t);
static void tcp_send_core(struct mbuf *, uint32_t, uint16_t, uint16_t, uint32_t, uint32_t, uint32_t, uint8_t, uint16_t);
static int is_seq_valid(uint32_t, uint32_t, uint32_t, uint16_t);
static int check_ack(struct sock_cb *, uint8_t, uint32_t, uint32_t, uint32_t);

struct rx_tcp_context {
	struct sock_cb *scb;
	struct mbuf *m;
	uint32_t raddr;
	uint16_t sport;
	uint16_t dport;
	uint16_t len;
};

void tcpinit() {
}

static uint16_t tcp_checksum(uint32_t ip_src, uint32_t ip_dst, uint8_t ip_p, struct tcp *tcphdr, uint16_t len) {
	uint32_t pseudo = 0;

	pseudo += ntohs((ip_src >> 16) & 0xffff);
	pseudo += ntohs(ip_src & 0xffff);
	pseudo += ntohs((ip_dst >> 16) & 0xffff);
	pseudo += ntohs(ip_dst & 0xffff);
	pseudo += (uint16_t)ip_p;
	pseudo += len;
	uint32_t res = cksum16((uint8_t *)tcphdr, len, pseudo);
	return res;
}

static int tcp_acquiresleep(struct sock_cb *scb) {
	acquire(&scb->lock);
	if (scb->state == SOCK_CB_ESTAB) {
		release(&scb->lock);
		return 0;
	}
	struct file *f = scb->f;
	if (f->nonblockable) {
		if (scb->state == SOCK_CB_SYN_RCVD) {
			release(&scb->lock);
			return 0;
		}
		release(&scb->lock);
		return -1;
	}
	release(&scb->lock);
	acquiresleep(&scb->slock);
	while (1) {
		struct proc *proc = myproc();
		if (proc != 0 && proc->killed){
			releasesleep(&scb->slock);
			return -1;
		}
		if (!holdingsleep(&scb->slock)) {
			break;
		}
	}
	return 0;
}

int tcp_listen(struct sock_cb *scb) {
	// scb->state = SOCK_CB_LISTEN;
	// TODO SOCK_CB_LISTEN STATE
	// -> change the connection from passive to active
	// "error: connection already exists"
	scb->state = SOCK_CB_LISTEN;
	return 0;
}

int tcp_accept(struct sock_cb* scb, uint32_t* raddr, uint16_t* dport) {
	if (tcp_acquiresleep(scb) == -1) {
		return -1;
	}
	*raddr = scb->raddr;
	*dport = scb->dport;

	return 0;
}

int tcp_connect(struct sock_cb *scb) {
	if (scb->socktype != SOCK_TCP) {
		printf("not tcp socket!\n");
		return -1;
	}
	struct mbuf *syn_m = mbufalloc(ETH_MAX_SIZE);
	scb->rcv.wnd = SOCK_CB_DEFAULT_WND_SIZE;
	tcp_send_core(syn_m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_SYN, 0);
	scb->state = SOCK_CB_SYN_SENT;
	scb->snd.nxt_seq = scb->snd.init_seq + 1;

	// TODO SOCK_CB_LISTEN STATE
	// -> chenge the connection from passive to active
	// "error: connection already exists"
	
	if (tcp_acquiresleep(scb) == -1) {
		return -1;
	}

	return 0;
}

static void tcp_send_core(
	struct mbuf *m,
	uint32_t raddr,
	uint16_t sport,
	uint16_t dport,
	uint32_t sndnxt,
	uint32_t rcvnxt,
	uint32_t rcvwnd,
	uint8_t flg,
	uint16_t payload_len
) {
	extern uint32_t local_ip;
	struct tcp *tcphdr;

	tcphdr = mbufpushhdr(m, *tcphdr);
	tcphdr->sport = htons(sport);
	tcphdr->dport = htons(dport);
	tcphdr->seq = htonl(sndnxt);
	if (TCP_FLG_ISSET(flg, TCP_FLG_ACK)) {
		tcphdr->ack = htonl(rcvnxt);
	} else {
		tcphdr->ack = 0;
	}
	tcphdr->off = (sizeof(struct tcp) >> 2) << 4;
	tcphdr->flg = flg;
	tcphdr->wnd = htons(rcvwnd);
	tcphdr->urg = 0;
	tcphdr->sum = 0;
	tcphdr->sum = htons(tcp_checksum(htonl(local_ip), htonl(raddr), IPPROTO_TCP, tcphdr, payload_len + sizeof(struct tcp)));
	ip_send(m, IPPROTO_TCP, raddr);
}

// https://tools.ietf.org/html/rfc793#page-56
int tcp_send(struct sock_cb *scb, struct mbuf *m, uint8_t flg) {
	if (scb == 0)
		return -1;
	if (m == 0)
		return -1;

	switch(scb->state) {
	case SOCK_CB_LISTEN:
		if (scb->raddr) {
			scb->snd.init_seq = 0;
			// TODO The urgent bit if requested in the command must be sent with the data segments sent
			// as a result of this command.
			tcp_send_core(m, scb->raddr, scb->sport, scb->dport, scb->snd.init_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_SYN, 0);
			scb->state = SOCK_CB_SYN_SENT;
			scb->snd.unack = scb->snd.init_seq;
			scb->snd.nxt_seq = scb->snd.init_seq+1;
			return 0;
		} else {
			// "error: foreign socket unspecified";
			return -1;
		}
		break;
	case SOCK_CB_SYN_SENT:
	case SOCK_CB_SYN_RCVD:
		push_to_scb_txq(scb, m, scb->snd.nxt_seq, flg, m->len);
		break;
	case SOCK_CB_ESTAB:
	// case SOCK_CB_CLOSE_WAIT:
		// TODO windows processing
		// send packet
		push_to_scb_txq(scb, m, scb->snd.nxt_seq, flg, m->len);
		struct mbuf *send_m = pop_from_scb_txq(scb);
		while(send_m) {
			// TODO buffer processing
			int sndnxt = send_m->params.tcp.sndnxt;
			int flg = send_m->params.tcp.flg;
			int len = send_m->params.tcp.datalen;
			tcp_send_core(send_m, scb->raddr, scb->sport, scb->dport, sndnxt, scb->rcv.nxt_seq, scb->rcv.wnd, flg, len);
			scb->snd.nxt_seq += len;
			send_m = pop_from_scb_txq(scb);
		}
		break;
	case SOCK_CB_CLOSE_WAIT:
	case SOCK_CB_FIN_WAIT_1:
	case SOCK_CB_FIN_WAIT_2:
	case SOCK_CB_CLOSING:
	case SOCK_CB_LAST_ACK:
	case SOCK_CB_TIME_WAIT:
	case SOCK_CB_CLOSED:
		// "error:  connection closing" and do not service request.
		return -1;
	default:
		return -1;
	}

	return 0;
}

int tcp_close(struct sock_cb *scb) {
	struct mbuf *m = 0;
	switch(scb->state) {
	case SOCK_CB_CLOSED:
		// "error:  connection illegal for this process".
		return 0;
	case SOCK_CB_LISTEN:
		// "error:  closing"
		return 0;
	case SOCK_CB_SYN_SENT:
		// "error closing"
		return 0;
	case SOCK_CB_SYN_RCVD:
		m = mbufalloc(ETH_MAX_SIZE);
		if (m == 0) {
			panic("[tcp close] m is zero");
		}
		if (!mbufq_empty(&scb->txq)) {
			tcp_send_core(m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_FIN | TCP_FLG_ACK, 0);
			scb->snd.nxt_seq += 1;
			scb->state = SOCK_CB_FIN_WAIT_1;
		} else {
			// TODO ... sequence number process
			push_to_scb_txq(scb, m, scb->snd.nxt_seq, TCP_FLG_FIN | TCP_FLG_ACK, m->len);
		}
		return -1;
	case SOCK_CB_ESTAB:
		m = mbufalloc(ETH_MAX_SIZE);
		tcp_send_core(m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_FIN | TCP_FLG_ACK, m->len);
		scb->snd.nxt_seq += 1;
		scb->state = SOCK_CB_FIN_WAIT_1;
		return -1;
	case SOCK_CB_FIN_WAIT_1:
	case SOCK_CB_FIN_WAIT_2:
		// "error: connection closing"
		return 0;
	case SOCK_CB_CLOSE_WAIT:
		m = mbufalloc(ETH_MAX_SIZE);
		tcp_send_core(m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_FIN | TCP_FLG_ACK, m->len);
		scb->snd.nxt_seq += 1;
		scb->state = SOCK_CB_LAST_ACK;
		return -1;
	default:
		return 0;
	}
	return 0;
}

int tcp_abort() {
	return -1;
}

static int is_seq_valid(uint32_t rcvwnd, uint32_t rcvnxt, uint32_t segseq, uint16_t seglen) {
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

static int tcp_recv_listen(struct sock_cb *scb, struct mbuf *m, uint32_t raddr, uint16_t dport, uint16_t len) {
	struct tcp *tcphdr = m->tcphdr;
	uint16_t sport = scb->sport;
	uint32_t ack = ntohl(tcphdr->ack);
	uint32_t seq = ntohl(tcphdr->seq);
	uint8_t flg = tcphdr->flg;

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

static int tcp_recv_syn_sent(struct sock_cb *scb, struct mbuf *m, uint16_t len) {
	struct tcp *tcphdr = m->tcphdr;
	uint32_t ack = ntohl(tcphdr->ack);
	uint32_t seq = ntohl(tcphdr->seq);
	uint8_t flg = tcphdr->flg;

	// SYN/ACK received
	if (TCP_FLG_ISSET(flg, TCP_FLG_ACK)) {
		if (ack <= scb->snd.init_seq || ack > scb->snd.nxt_seq) {
			struct mbuf *rst_ack_m = mbufalloc(ETH_MAX_SIZE);
			tcp_send_core(rst_ack_m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_RST | TCP_FLG_ACK, 0);
			scb->snd.nxt_seq = scb->snd.init_seq + 1;
			return TCP_OP_ERR;
		}
	}
	if (TCP_FLG_ISSET(flg, TCP_FLG_RST)) {
		// TODO free sock
		return TCP_OP_ERR;
	}
	// TODO check the security and precedence

	if (TCP_FLG_ISSET(flg, TCP_FLG_SYN) && TCP_FLG_ISSET(flg, TCP_FLG_ACK)) {
		scb->rcv.nxt_seq = seq + 1;
		scb->rcv.init_seq = seq;
		scb->snd.unack = ack;

		if (scb->snd.unack > scb->snd.init_seq) {
			scb->state = SOCK_CB_ESTAB;
			releasesleep(&scb->slock);
			struct mbuf *ack_m = mbufalloc(ETH_MAX_SIZE);
			tcp_send_core(ack_m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_ACK, 0);
		} else {
			scb->state = SOCK_CB_SYN_RCVD;
			struct mbuf *syn_ack_m = mbufalloc(ETH_MAX_SIZE);
			tcp_send_core(syn_ack_m, scb->raddr, scb->sport, scb->dport, scb->snd.init_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_SYN | TCP_FLG_ACK, 0);
		}
	} else {
		return TCP_OP_ERR;
	}
		return TCP_OP_OK;
}

static int check_rst(struct sock_cb *scb, uint8_t flg) {
	if (TCP_FLG_ISSET(flg, TCP_FLG_RST)) {
		switch(scb->state) {
		case SOCK_CB_SYN_RCVD:
			scb->state = SOCK_CB_LISTEN;
			return TCP_OP_CLOSE_SCB;
		case SOCK_CB_ESTAB:
		case SOCK_CB_FIN_WAIT_1:
		case SOCK_CB_FIN_WAIT_2:
		case SOCK_CB_CLOSE_WAIT:
			// TODO
			// segment queues should be flushed
			// close scb
			scb->state = SOCK_CB_CLOSED;
			return TCP_OP_CLOSE_SCB;
		case SOCK_CB_CLOSING:
		case SOCK_CB_LAST_ACK:
		case SOCK_CB_TIME_WAIT:
			// close scb
			scb->state = SOCK_CB_CLOSED;
			return TCP_OP_CLOSE_SCB;
		default:
			break;
		}
	}
	return 0;
}

static int check_ack(struct sock_cb *scb, uint8_t flg, uint32_t ack, uint32_t seq, uint32_t sndwnd) {
	if (!TCP_FLG_ISSET(flg, TCP_FLG_ACK)) {
		return TCP_OP_ERR;
	}

	int is_valid_nxtack   = scb->snd.unack < ack && ack <= scb->snd.nxt_seq;
	int is_old_ack        = ack < scb->snd.unack;
	int is_future_ack     = scb->snd.nxt_seq < ack;

	switch(scb->state) {
	case SOCK_CB_SYN_RCVD:
		if (is_valid_nxtack) {
			scb->state = SOCK_CB_ESTAB;
			releasesleep(&scb->slock);
		} else {
			return TCP_OP_SND_RST;
		}
		break;
	case SOCK_CB_ESTAB:
	case SOCK_CB_FIN_WAIT_1:
	case SOCK_CB_FIN_WAIT_2:
	case SOCK_CB_CLOSE_WAIT:
	case SOCK_CB_CLOSING:
		if (is_valid_nxtack) {
			scb->snd.unack = ack;
			// Any segments on the retransmission queue which are thereby entirely
			// acknowledged are removed
			int can_update_sndwnd = scb->snd.wl1 < seq || (scb->snd.wl1 == seq && scb->snd.wl2 <= ack);
			if (can_update_sndwnd) {
			scb->snd.wnd = sndwnd;
			scb->snd.wl1 = seq;
			scb->snd.wl2 = ack;
			}
		} else if (is_old_ack) {
			// ignore
			return TCP_OP_ERR;
		} else if (is_future_ack) {
			return TCP_OP_SND_ACK;
		}

		// state transition
		if (scb->state == SOCK_CB_FIN_WAIT_1) {
			scb->state = SOCK_CB_FIN_WAIT_2;
		}
		if (scb->state == SOCK_CB_FIN_WAIT_2) {
			// TODO check whether retransmission queue is empty
		}
		if (scb->state == SOCK_CB_CLOSING) {
			scb->state = SOCK_CB_CLOSED;
			return TCP_OP_CLOSE_SCB;
		}
		break;
	case SOCK_CB_LAST_ACK:
		scb->state = SOCK_CB_CLOSED;
		return TCP_OP_CLOSE_SCB;
	case SOCK_CB_TIME_WAIT:
		// TODO
		// The only thing that can arrive in this state is a 
		// retransmission of the remote FIN. Acknowledge it, and restart
		// the 2 MSL timeout
		break;
	default:
		break;
	}
	return TCP_OP_OK;
}

static int check_syn(struct sock_cb *scb, uint8_t flg) {
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
		if (TCP_FLG_ISSET(flg, TCP_FLG_SYN)) {
			return TCP_OP_SND_RST;
		}
	}
	return TCP_OP_OK;
}

static int check_text(struct sock_cb *scb, uint32_t seq, uint16_t datalen) {
	switch(scb->state) {
	case SOCK_CB_ESTAB:
	case SOCK_CB_FIN_WAIT_1:
	case SOCK_CB_FIN_WAIT_2:
		if (datalen > 0 && scb->rcv.nxt_seq == seq) {
			if (scb->rcv.wnd > datalen) {
				scb->rcv.nxt_seq += datalen;
				scb->rcv.wnd -= datalen;
				return TCP_OP_TXT_OK;
			} else {
				return TCP_OP_RETRANS;
			}
		}
		break;
	case SOCK_CB_CLOSE_WAIT:
	case SOCK_CB_CLOSING:
	case SOCK_CB_LAST_ACK:
	case SOCK_CB_TIME_WAIT:
		// This should not occur, since a FIN has been received from the
		// remote side. Ignore the segment text.
		return TCP_OP_ERR;
	default:
		return TCP_OP_ERR;
	}
	return TCP_OP_OK;
}

static int check_fin(struct sock_cb *scb, uint8_t flg) {
	if (TCP_FLG_ISSET(flg, TCP_FLG_FIN)) {
		switch(scb->state) {
		case SOCK_CB_CLOSED:
		case SOCK_CB_LISTEN:
		case SOCK_CB_SYN_SENT:
			break;
		// signal the user "connection closing"
		// return any pending RECEIVEs with above message
		// advance RCV.NXT over the FIN
		// send ack for the FIN
		// Note that FIN implies PUSH for any segment text not yet delivered to the user.
		case SOCK_CB_SYN_RCVD:
		case SOCK_CB_ESTAB:
			scb->rcv.nxt_seq += 1;
			scb->state = SOCK_CB_CLOSE_WAIT;
			return TCP_OP_SND_ACK;
		case SOCK_CB_FIN_WAIT_1:
			// TODO
			// If our FIN has been ACKed, then enter TIME_WAIT, start the time-wait timer
			// otherwise enter the CLOSING state;
			scb->rcv.nxt_seq += 1;
			return TCP_OP_CLOSE_SCB;
		case SOCK_CB_FIN_WAIT_2:
			scb->rcv.nxt_seq += 1;
			return TCP_OP_CLOSE_SCB;
		case SOCK_CB_CLOSE_WAIT:
		case SOCK_CB_CLOSING:
		case SOCK_CB_LAST_ACK:
		case SOCK_CB_TIME_WAIT:
			// remain in the CLOSE_WAIT
			// If state is TIME_WAIT, restart the 2 MSL time_wait timeout.
			break;
		}
		return TCP_OP_ERR;
	}
	return TCP_OP_OK;
}

static int tcp_recv_core(struct sock_cb *scb, struct mbuf *m, uint16_t len) {
	struct tcp *tcphdr = m->tcphdr;
	uint32_t ack = ntohl(tcphdr->ack);
	uint32_t seq = ntohl(tcphdr->seq);
	uint8_t flg = tcphdr->flg;
	uint16_t datalen = TCP_DATA_LEN(tcphdr, len);
	
	struct mbuf *snd_m = 0;

	// first check sequence number
	if (!is_seq_valid(scb->rcv.wnd, scb->rcv.nxt_seq, seq, datalen)) {
		// send ack(unless the RST bit is set, if so drop the segment and return)
		if (!TCP_FLG_ISSET(flg, TCP_FLG_RST)) {
			snd_m = mbufalloc(ETH_MAX_SIZE);
			tcp_send_core(snd_m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_ACK, 0);
		}
		return TCP_OP_ERR;
	}

	// second check the RST bit
	switch (check_rst(scb, flg)) {
	case TCP_OP_CLOSE_SCB:
		return TCP_OP_CLOSE_SCB;
	case TCP_OP_ERR:
		return TCP_OP_ERR;
	default:
		break;
	}
	
	// TODO third check security and precednece
	
	// fource, check the SYN bit
	if (check_syn(scb, flg) == TCP_OP_SND_RST) {
		snd_m = mbufalloc(ETH_MAX_SIZE);
		tcp_send_core(snd_m, scb->raddr, scb->sport, scb->dport, ack, seq, scb->rcv.wnd, TCP_FLG_RST, 0);
		return TCP_OP_CLOSE_SCB;
	}
	
	// fifth check the ACK field
	switch (check_ack(scb, flg, ack, seq, ntohl(tcphdr->wnd))) {
	case TCP_OP_CLOSE_SCB:
		return TCP_OP_CLOSE_SCB;
	case TCP_OP_SND_RST:
		snd_m = mbufalloc(ETH_MAX_SIZE);
		tcp_send_core(snd_m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_RST, 0);
		return TCP_OP_ERR;
	case TCP_OP_ERR:
		return TCP_OP_ERR;
	case TCP_OP_OK:
		break;
	case TCP_OP_SND_ACK:
		snd_m = mbufalloc(ETH_MAX_SIZE);
		tcp_send_core(snd_m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_ACK, 0);
		return TCP_OP_ERR;
	default:
		break;
	}
	
	// TODO sixth, check the URG bit

	// seventh, process the segment text
	// deliver segment text to user RECEIVE buffers
	switch (check_text(scb, seq, datalen)) {
	case TCP_OP_ERR:
		return TCP_OP_ERR;
	case TCP_OP_TXT_OK:
		m->len = datalen;
		m->params.tcp.flg = tcphdr->flg;
		push_to_scb_rxq(scb, m);
		snd_m = mbufalloc(ETH_MAX_SIZE);
		tcp_send_core(snd_m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_ACK, 0);
		return TCP_OP_OK;
	default:
		break;
	}
	

	// eighth, check the FIN bit
	switch (check_fin(scb, flg)) {
	case TCP_OP_RETRANS:
		snd_m = mbufalloc(ETH_MAX_SIZE);
		tcp_send_core(snd_m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_ACK, 0);
		return TCP_OP_OK;
	case TCP_OP_CLOSE_SCB:
		snd_m = mbufalloc(ETH_MAX_SIZE);
		tcp_send_core(snd_m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_ACK, 0);
		return TCP_OP_CLOSE_SCB;
	case TCP_OP_SND_ACK: 
		snd_m = mbufalloc(ETH_MAX_SIZE);
		tcp_send_core(snd_m, scb->raddr, scb->sport, scb->dport, scb->snd.nxt_seq, scb->rcv.nxt_seq, scb->rcv.wnd, TCP_FLG_ACK, 0);
		return TCP_OP_ERR;
	default:
		break;
	}
	return TCP_OP_FREE_MBUF;
}

// segment arrives
void tcp_recv(struct mbuf *m, uint16_t len, struct ipv4 *iphdr) {
	struct sock_cb *scb = 0;
	// pull header
	struct tcp *tcphdr = mbufpullhdr(m, *tcphdr);
	if (!tcphdr)
		goto fail;
	m->tcphdr = tcphdr;
	uint32_t raddr = ntohl(iphdr->ip_src);
	uint16_t sport = ntohs(tcphdr->dport);
	uint16_t dport = ntohs(tcphdr->sport);
	
	// checksum
	uint16_t sum = tcp_checksum(iphdr->ip_src, iphdr->ip_dst, iphdr->ip_p, tcphdr, len);
	if (sum) {
		printf("[bad tcp] checksum doesn't match, %d, len: %d\n", sum, len);
		goto fail;
	}

	// get scb
	scb = get_sock_cb(tcp_scb_table, sport, raddr, dport);
	if (scb == 0) {
		goto fail;
	}

	printf("%p recv_state: %d\n", scb, scb->state);

	// early return
	if (scb->state != SOCK_CB_LISTEN && scb->state != SOCK_CB_CLOSED && scb->dport != dport) {
		goto fail;
	}

	// state processing
	switch(scb->state) {
	case SOCK_CB_CLOSED:
		// TODO if a incoming packet does not contain a RST, send a RST packet.
		goto fail;
		break;
	case SOCK_CB_LISTEN:
		if (tcp_recv_listen(scb, m, raddr, dport, len) == TCP_OP_ERR)
			goto fail;
		break;
	case SOCK_CB_SYN_SENT:
		if (tcp_recv_syn_sent(scb, m, len) == -1)
			goto fail;
		break;
	case SOCK_CB_SYN_RCVD:
	case SOCK_CB_ESTAB:
	case SOCK_CB_FIN_WAIT_1:
	case SOCK_CB_FIN_WAIT_2:
	case SOCK_CB_CLOSE_WAIT:
	case SOCK_CB_CLOSING:
	case SOCK_CB_LAST_ACK:
	case SOCK_CB_TIME_WAIT:
		switch (tcp_recv_core(scb, m, len)) {
		case TCP_OP_CLOSE_SCB:
			free_sock_cb(scb);
			goto fail;
		case TCP_OP_ERR:
		case TCP_OP_FREE_MBUF:
			goto fail;
		default:
			break;
		}
		break;
	}

	// TODO URG process
	return;
fail:
	return;
}

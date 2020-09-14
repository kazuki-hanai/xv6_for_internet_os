#include "defs.h"
#include "net/mbuf.h"
#include "net/netutil.h"
#include "net/sock_cb.h"
#include "net/socket.h"
#include "file.h"
#include "lib/buddy.h"

struct sock_cb_entry tcp_scb_table[SOCK_CB_LEN];
struct sock_cb_entry udp_scb_table[SOCK_CB_LEN];

struct sock_cb* alloc_sock_cb(struct file *f, uint32_t raddr, uint16_t sport, uint16_t dport, int socktype) {
	struct sock_cb *scb;
	scb = ufkalloc(sizeof(struct sock_cb));
	if (scb == 0)
		panic("[alloc_sock_cb] could not allocate\n");
	memset(scb, 0, sizeof(*scb));
	scb->f = f;
	scb->state = SOCK_CB_CLOSED;
	initlock(&scb->lock, "scb lock");
	initsleeplock(&scb->slock, "scb sleep lock");
	scb->socktype = socktype;
	scb->raddr = raddr;
	scb->sport = sport;
	scb->dport = dport;
	scb->prev = 0;
	scb->next = 0;
	scb->wnd = kalloc();
	scb->wnd_idx = 0;
	
	scb->snd.init_seq = 0;
	scb->snd.nxt_seq = 0;
	scb->snd.unack = 0;
	scb->snd.wl1 = 0;
	scb->snd.wl2 = 0;
	scb->snd.wnd = 0;

	scb->rcv.init_seq = 0;
	scb->rcv.nxt_seq = 0;
	scb->rcv.wnd = PGSIZE;

	mbufq_init(&scb->txq);
	mbufq_init(&scb->rxq);
	return scb;
}

void free_sock_cb(struct sock_cb *scb) {
	if (scb != 0) {
		struct sock_cb_entry *entry;
		if (scb->socktype == SOCK_TCP) {
			entry = &tcp_scb_table[scb->sport % SOCK_CB_LEN];
		} else {
			entry = &udp_scb_table[scb->sport % SOCK_CB_LEN];
		}

		if (scb->f) {
			filefree(scb->f);
		}

		kfree(scb->wnd);

		struct mbuf *m;
		while((m = pop_from_scb_rxq(scb)) != 0) {
			mbuffree(m);
		}
		while((m = pop_from_scb_txq(scb)) != 0) {
			mbuffree(m);
		}

		release_sport(scb->sport);
		acquire(&entry->lock);
		if (scb->next != 0)
			scb->next->prev = scb->prev;
		if (scb->prev != 0)
			scb->prev->next = scb->next;
		else
			entry->head = scb->next;
		ufkfree(scb);
		release(&entry->lock);
	}
}

void add_sock_cb(struct sock_cb *scb) {
	if (scb == 0) {
		return;
	}

	struct sock_cb_entry *entry;
	
	if (scb->socktype == SOCK_TCP) {
		entry = &tcp_scb_table[scb->sport % SOCK_CB_LEN];
	} else {
		entry = &udp_scb_table[scb->sport % SOCK_CB_LEN];
	}

	acquire(&entry->lock);
	struct sock_cb *tail = entry->head;
	if (tail == 0) {
		entry->head = scb;
		scb->prev = 0;
		scb->next = 0;
	} else {
		while (tail->next != 0) {
			tail = tail->next;
		}
		tail->next = scb;
		scb->prev = tail;
		scb->next = 0;
	}
	release(&entry->lock);
}

struct sock_cb* get_sock_cb(struct sock_cb_entry table[], uint16_t sport, uint16_t dport) {
	struct sock_cb_entry* entry;
	struct sock_cb *scb;

	entry = &table[sport % SOCK_CB_LEN];

	acquire(&entry->lock);
	scb = entry->head;
	while (scb != 0) {
		if (scb->sport == sport && scb->dport != 0 && scb->dport == dport)
			break;
		scb = scb->next;
	}
	release(&entry->lock);
	return scb;
}

int
push_to_scb_rxq(struct sock_cb *scb, struct mbuf *m)
{
	if (scb == 0) {
		printf("scb: %d\n", scb);
		return -1;
	}
	acquire(&scb->lock);
	mbufq_pushtail(&scb->rxq, m);
	release(&scb->lock);
	return 0;
}

struct mbuf *pop_from_scb_rxq(struct sock_cb *scb) {
	if (scb == 0) {
		printf("scb: %d\n", scb);
		return 0;
	}
	acquire(&scb->lock);
	struct mbuf *m = mbufq_pophead(&scb->rxq);
	release(&scb->lock);
	return m;
}

int
push_to_scb_txq(struct sock_cb *scb, struct mbuf *m, uint32_t sndnxt, uint8_t flg, uint16_t datalen)
{
	if (scb == 0) {
		return -1;
	}

	m->params.tcp.sndnxt = sndnxt;
	m->params.tcp.flg = flg;
	m->params.tcp.datalen = datalen;

	acquire(&scb->lock);
	mbufq_pushtail(&scb->txq, m);
	release(&scb->lock);
	return 0;
}

struct mbuf *pop_from_scb_txq(struct sock_cb *scb) {
	if (scb == 0) {
		printf("scb: %d\n", scb);
		return 0;
	}
	acquire(&scb->lock);
	struct mbuf *m = mbufq_pophead(&scb->txq);
	release(&scb->lock);
	return m;
}

#pragma once

#include "arch/riscv.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "net/mbuf.h"

#define SOCK_CB_LEN 128

enum sock_cb_state {
	SOCK_CB_CLOSED,
	SOCK_CB_LISTEN,
	SOCK_CB_SYN_SENT,
	SOCK_CB_SYN_RCVD,
	SOCK_CB_ESTAB,
	SOCK_CB_FIN_WAIT_1,
	SOCK_CB_FIN_WAIT_2,
	SOCK_CB_CLOSING, 
	SOCK_CB_TIME_WAIT,
	SOCK_CB_CLOSE_WAIT,
	SOCK_CB_LAST_ACK
};

#define SOCK_UNKNOWN 0
#define SOCK_UDP 1
#define SOCK_TCP 2

#define SOCK_CB_DEFAULT_WND_SIZE PGSIZE

/**
 * Struct connecting Socket and Tcb
 **/
struct sock_cb {
  struct file *f;
  struct sleeplock slock;
  struct spinlock lock;
  int socktype;
  struct sock_cb* acpt_scb;
  enum sock_cb_state state;
  uint16_t sport;
  uint32_t raddr;
  uint16_t dport;
  struct {
    uint32_t init_seq; // initial send sequence number
    uint32_t unack; // oldest unacknowledged sequence number
    uint32_t nxt_seq; // next sequence number to be sent
    uint32_t wnd;
    uint32_t wl1;
    uint32_t wl2;
  } snd;
  struct {
    uint32_t init_seq; // initial receive sequence number
    uint32_t nxt_seq; // next sequence number to be sent
    uint32_t wnd;
  } rcv;
  struct mbufq txq;
  struct mbufq rxq;
  struct sock_cb *prev;
  struct sock_cb *next;
  uint8_t *wnd;
  int wnd_idx;
};

struct sock_cb_entry {
	struct spinlock lock;
	struct sock_cb *head;
};

struct sock_cb* alloc_sock_cb(struct file *, uint32_t, uint16_t, uint16_t, int);
void free_sock_cb(struct sock_cb *);
void add_sock_cb(struct sock_cb *);
struct sock_cb* get_sock_cb(struct sock_cb_entry [], uint16_t, uint32_t, uint16_t);
int push_to_scb_rxq(struct sock_cb *, struct mbuf *);
struct mbuf *pop_from_scb_rxq(struct sock_cb *scb);
int push_to_scb_txq(struct sock_cb *, struct mbuf *, uint32_t sndnxt, uint8_t flg, uint16_t datalen);
struct mbuf *pop_from_scb_txq(struct sock_cb *scb);
int sock_cb_acquiresleep(struct sock_cb *scb);

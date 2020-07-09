#pragma once

#include "arch/riscv.h"
#include "spinlock.h"
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
  struct spinlock lock;
  int socktype;
  enum sock_cb_state state;
  uint16 sport;
  uint32 raddr;
  uint16 dport;
  struct {
    uint32 init_seq; // initial send sequence number
    uint32 unack; // oldest unacknowledged sequence number
    uint32 nxt_seq; // next sequence number to be sent
    uint32 wnd;
    uint32 wl1;
    uint32 wl2;
  } snd;
  struct {
    uint32 init_seq; // initial receive sequence number
    uint32 unack; // oldest unacknowledged sequence number
    uint32 nxt_seq; // next sequence number to be sent
    uint32 wnd;
  } rcv;
  struct mbufq txq;
  struct mbufq rxq;
  // uint8 window[TCP_DEFAULT_WINDOW];
  struct sock_cb *prev;
  struct sock_cb *next;
  uint8 *wnd;
  int wnd_idx;
};

struct sock_cb_entry {
  struct spinlock lock;
  struct sock_cb *head;
};

struct sock_cb* init_sock_cb(uint32, uint16, uint16, int);
void free_sock_cb(struct sock_cb *);
void add_sock_cb(struct sock_cb_entry [], struct sock_cb *);
struct sock_cb* get_sock_cb(struct sock_cb_entry [], uint16);
int push_to_scb_rxq(struct sock_cb *, struct mbuf *);
int push_to_scb_txq(struct sock_cb *, struct mbuf *);
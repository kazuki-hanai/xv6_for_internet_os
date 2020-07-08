#pragma once

#include "spinlock.h"
#include "net/mbuf.h"

#define SOCK_CB_LEN 128

enum sock_cb_state {
  CLOSED,
  LISTEN,
  SYN_SENT,
  SYN_RCVD,
  ESTAB,
  FIN_WAIT_1,
  FIN_WAIT_2,
  CLOSING, TIME_WAIT,
  CLOSE_WAIT,
  LAST_ACK
};

#define SOCK_UNKNOWN 0
#define SOCK_UDP 1
#define SOCK_TCP 2

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
};

struct sock_cb_entry {
  struct spinlock lock;
  struct sock_cb *head;
};

struct sock_cb* init_sock_cb(uint32, uint16, uint16, int);
void free_sock_cb(struct sock_cb *);
void add_sock_cb(struct sock_cb_entry [], struct sock_cb *);
struct sock_cb* get_sock_cb(struct sock_cb_entry [], uint16);
int push_to_scb_rxq(struct sock_cb_entry [], struct mbuf *, uint32, uint16, uint16);
int push_to_scb_txq(struct sock_cb_entry [], struct mbuf *, uint32, uint16, uint16);
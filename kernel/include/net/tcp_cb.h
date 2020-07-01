#pragma once
#define TCP_CB_LEN 128

enum tcp_cb_state {
  CLOSED,
  LISTEN,
  SYN_SENT,
  SYN_RCVD,
  ESTAB,
  FIN_WAIT_1,
  FIN_WAIT_2,
  CLOSING,
  TIME_WAIT,
  CLOSE_WAIT,
  LAST_ACK
};

struct tcp_cb {
  struct spinlock lock;
  enum tcp_cb_state state;
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
  // uint8 window[TCP_DEFAULT_WINDOW];
  struct tcp_cb *prev;
  struct tcp_cb *next;
};

struct tcp_cb_entry {
  struct spinlock lock;
  struct tcp_cb *head;
};


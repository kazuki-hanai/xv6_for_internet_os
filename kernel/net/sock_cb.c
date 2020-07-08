#include "defs.h"
#include "net/mbuf.h"
#include "net/netutil.h"
#include "net/sock_cb.h"

struct sock_cb_entry tcp_scb_table[SOCK_CB_LEN];
struct sock_cb_entry udp_scb_table[SOCK_CB_LEN];

struct sock_cb* init_sock_cb(uint32 raddr, uint16 sport, uint16 dport, int socktype) {
  struct sock_cb *scb;
  scb = bd_alloc(sizeof(struct sock_cb));
  if (scb == 0)
    panic("[init_sock_cb] could not allocate\n");
  memset(scb, 0, sizeof(*scb));
  scb->state = CLOSED;
  initlock(&scb->lock, "scb lock");
  scb->socktype = socktype;
  scb->raddr = raddr;
  scb->sport = sport;
  scb->dport = dport;
  scb->prev = 0;
  scb->next = 0;
  mbufq_init(&scb->txq);
  mbufq_init(&scb->rxq);
  return scb;
}

void free_sock_cb(struct sock_cb_entry table[], struct sock_cb *scb) {
  if (scb != 0) {
    struct sock_cb_entry *entry;
    entry = &table[scb->sport % SOCK_CB_LEN];

    acquire(&entry->lock);
    if (scb->next != 0)
      scb->next->prev = scb->prev;
    if (scb->prev != 0)
      scb->prev->next = scb->next;
    else
      entry->head = scb->next;
    bd_free(scb);
    release(&entry->lock);
  }
}

void add_sock_cb(struct sock_cb_entry table[], struct sock_cb *scb) {
  if (scb == 0) {
    return;
  }

  struct sock_cb_entry *entry = &table[scb->sport % SOCK_CB_LEN];

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

struct sock_cb* get_sock_cb(struct sock_cb_entry table[], uint16 sport) {
  struct sock_cb_entry* entry;
  struct sock_cb *scb;

  entry = &table[sport % SOCK_CB_LEN];

  acquire(&entry->lock);
  scb = entry->head;
  while (scb != 0) {
    if (scb->sport == sport)
      break;
    scb = scb->next;
  }
  release(&entry->lock);
  return scb;
}

int
push_to_scb_rxq(struct sock_cb_entry table[], struct mbuf *m, uint32 raddr, uint16 sport, uint16 dport)
{
  struct sock_cb *scb = get_sock_cb(table, sport);
  if (scb == 0) {
    printf("scb: %d\n", scb);
    return -1;
  }
  acquire(&scb->lock);
  mbufq_pushtail(&scb->rxq, m);
  release(&scb->lock);
  return 0;
}

int
push_to_scb_txq(struct sock_cb_entry table[], struct mbuf *m, uint32 raddr, uint16 sport, uint16 dport)
{
  struct sock_cb *scb = get_sock_cb(table, sport);
  if (scb == 0) {
    return -1;
  }
  acquire(&scb->lock);
  mbufq_pushtail(&scb->txq, m);
  release(&scb->lock);
  return 0;
}
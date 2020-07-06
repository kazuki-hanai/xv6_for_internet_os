#include "defs.h"
#include "net/mbuf.h"
#include "net/netutil.h"
#include "net/sock_cb.h"

struct sock_cb_entry scb_table[TCP_CB_LEN];

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
  return scb;
}

void free_sock_cb(struct sock_cb *scb) {
  if (scb != 0) {
    struct sock_cb_entry *entry;
    uint16 port;
    if (socktype == SOCK_UDP || socktype == SOCK_TCP)
      port = scb->dport;
    else
      port = scb->sport;
    entry = &scb_table[(raddr + port) % TCP_CB_LEN];

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

struct sock_cb* get_sock_cb(uint32 raddr, uint16 sport, uint16 dport, int socktype) {
  struct sock_cb_entry* entry;
  struct sock_cb *scb;
  struct sock_cb *prev;
  uint16 port;

  if (socktype == SOCK_UDP || socktype == SOCK_TCP)
    port = dport;
  else
    port = sport;

  entry = &scb_table[(raddr + port) % TCP_CB_LEN];

  acquire(&entry->lock);
  scb = entry->head;
  prev = 0;
  while (scb != 0) {
    if (scb->raddr == raddr && scb->sport == sport && scb->dport == dport)
      break;
    prev = scb;
    scb = scb->next;
  }
  
  // new scb
  if(scb == 0) {
    scb = init_sock_cb(raddr, sport, dport);
    if (prev != 0)
      prev->next = scb;
    scb->prev = prev;
  // Already exists
  } else if (
    scb != 0 && 
    scb->raddr == raddr &&
    scb->sport == sport &&
    scb->dport == dport
  ){ 

  } else {
    panic("[get_scb] invalid!\n");
  }

  if (entry->head == 0)
    entry->head = scb;
  
  release(&entry->lock);
  return scb;
}
#include "defs.h"
#include "net/mbuf.h"
#include "net/netutil.h"
#include "net/tcp_cb.h"

struct tcp_cb_entry tcb_table[TCP_CB_LEN];

struct tcp_cb* init_tcp_cb(uint32 raddr, uint16 sport, uint16 dport) {
  struct tcp_cb *tcb;
  tcb = bd_alloc(sizeof(struct tcp_cb));
  if (tcb == 0)
    panic("[init_tcp_cb] could not allocate\n");
  memset(tcb, 0, sizeof(*tcb));
  tcb->state = CLOSED;
  initlock(&tcb->lock, "tcb lock");
  tcb->raddr = raddr;
  tcb->sport = sport;
  tcb->dport = dport;
  tcb->prev = 0;
  tcb->next = 0;
  return tcb;
}

void free_tcp_cb(struct tcp_cb *tcb) {
  if (tcb != 0) {
    struct tcp_cb_entry *entry = &tcb_table[(tcb->raddr + (tcb->sport << 16) + tcb->dport) % TCP_CB_LEN];
    acquire(&entry->lock);
    if (tcb->next != 0)
      tcb->next->prev = tcb->prev;
    if (tcb->prev != 0)
      tcb->prev->next = tcb->next;
    else
      entry->head = tcb->next;
    bd_free(tcb);
    release(&entry->lock);
  }
}

struct tcp_cb* get_tcb(uint32 raddr, uint16 sport, uint16 dport) {
  struct tcp_cb_entry* entry;
  struct tcp_cb *tcb;
  struct tcp_cb *prev;
  entry = &tcb_table[(raddr + (sport << 16) + dport) % TCP_CB_LEN];

  acquire(&entry->lock);
  tcb = entry->head;
  prev = 0;
  while (tcb != 0) {
    if (tcb->raddr == raddr && tcb->sport == sport && tcb->dport == dport)
      break;
    prev = tcb;
    tcb = tcb->next;
  }
  
  // new tcb
  if(tcb == 0) {
    tcb = init_tcp_cb(raddr, sport, dport);
    if (prev != 0)
      prev->next = tcb;
    tcb->prev = prev;
  // Already exists
  } else if (
    tcb != 0 && 
    tcb->raddr == raddr &&
    tcb->sport == sport &&
    tcb->dport == dport
  ){ 

  } else {
    panic("[get_tcb] invalid port!\n");
  }

  if (entry->head == 0)
    entry->head = tcb;
  
  release(&entry->lock);
  return tcb;
}
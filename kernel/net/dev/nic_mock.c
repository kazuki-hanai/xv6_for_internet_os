#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "net/mbuf.h"
#include "net/ethernet.h"

struct mbufq tx_queue;
struct spinlock tx_lock;

void
e1000_init(uint32 *xregs)
{
  initlock(&tx_lock, "tx lock");
  mbufq_init(&tx_queue);
}

int
e1000_transmit(struct mbuf *m)
{
  // push mbuf to queue
  acquire(&tx_lock);
  mbufq_pushtail(&tx_queue, mbuf_copy(m));
  release(&tx_lock);
  
  return 0;
}

static void
e1000_recv(void)
{
  // no process
}

void 
nic_mock_recv(struct mbuf *m) {
  eth_recv(m);
}

void 
e1000_intr() {
  e1000_recv();
}


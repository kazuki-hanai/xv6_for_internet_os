//
// network system calls.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "net/netutil.h"
#include "net/tcp.h"
#include "net/sock_cb.h"
#include "sys/sysnet.h"

struct sock_cb_entry scb_table[TCP_CB_LEN];

struct spinlock sport_lock;
uint16 current_sport = START_OF_SPORT;

uint16 get_new_sport() {
  int islooped = 0, i;
  acquire(&sport_lock);
  while(1) {
    if (sport_table[current_sport] != 0xff) {
      for (i = 1; sport_table[current_sport] & i == 0; i++);
      break;
    }
    if (curren_sport == SPORT_NUM-1) {
      current_sport = START_OF_SPORT;
      if (islooped == 0) {
        islooped += 1;
      } else {
        panic("[get_new_sport] sport is full");
      }
    } else {
      current_sport += 1;
    }
  }
  release(&sport_lock);
  return START_OF_SPORT + current_sport * 8 + i;
}

void release_sport(uint16) {
  
}

void
sockinit(void)
{
  initlock(&sport_lock, "sportlock");
  memset(scb_table, 0, sizeof(scb_table));
  for (int i = 0; i < TCP_CB_LEN; i++) {
    scb_table[i].head = 0;
    initlock(&scb_table[i].lock, "scb entry lock");
  }
}

int
sockalloc(struct file **f, uint32 raddr, uint16 sport, uint16 dport, int stype)
{
  struct sock *si;

  si = 0;
  *f = 0;
  if ((*f = filealloc()) == 0)
    goto bad;
  if ((si = (struct sock*)kalloc()) == 0)
    goto bad;

  if (stype == SOCK_TCP || stype == SOCK_UDP) {
    sport = 0;
  } else {
    dport = 0;
  }
  // initialize objects
  si->raddr = raddr;
  si->sport = sport;
  si->dport = dport;
  si->stype = stype;

  if(stype == SOCK_TCP || stype == SOCK_TCP_LISTEN) {
    si->scb = tcp_open(raddr, sport, dport, stype);
    if (si->scb == 0) {
      goto bad;
    }
  } else {
    si->scb = 0;
  }

  if(stype == SOCK_TCP_LISTEN || stype == SOCK_UDP_LISTEN) {
    si->dport = 0;
  }

  initlock(&si->lock, "sock");
  mbufq_init(&si->rxq);
  (*f)->type = FD_SOCK;
  (*f)->readable = 1;
  (*f)->writable = 1;
  (*f)->sock = si;

  // add to list of sockets
  // acquire(&lock);
  // pos = sockets;
  // while (pos) {
  //   if (pos->raddr == raddr &&
  //       pos->sport == sport &&
  //       pos->dport == dport
  //   ) {
  //     release(&lock);
  //     goto bad;
  //   }
  //   pos = pos->next;
  // }
  // si->next = sockets;
  // sockets = si;
  // release(&lock);
  return 0;

bad:
  if (si)
    kfree((char*)si);
  if (*f)
    fileclose(*f);
  return -1;
}

void sockfree(struct sock *si) {
  kfree((char*)si);
}

// called by protocol handler layer to deliver UDP packets
void
sockrecvudp(struct mbuf *m, uint32 raddr, uint16 sport, uint16 dport)
{
  // acquire(&lock);
  // struct sock *sock;
  // sock = sockets;
  // while (sock) {
  //   if (sock->raddr == raddr &&
  //       sock->sport == sport &&
  //       sock->dport == dport) {
  //     release(&lock);
  //     acquire(&sock->lock);
  //     mbufq_pushtail(&sock->rxq, m);
  //     release(&sock->lock);
  //     return;
  //   }
  //   sock = sock->next;
  // }
  // release(&lock);
  mbuffree(m);
}

uint64
sys_socket(void)
{
  struct file *f;
  int stype;

  if(
    argint(1, (int *)&stype) < 0
  )
    return -1;

  int fd;
  if(sockalloc(&f, (uint32)raddr, (uint16)sport, (uint16)dport, stype) != 0 || (fd = fdalloc(f)) < 0)
    return -1;

  return fd;
}

void sys_sockconn() {

}

void sys_socklisten() {

}

// UDP only now
int
sys_socksend(struct file *f, uint64 addr, int n)
{
  // TODO split data
  struct sock *s = f->sock;
  struct mbuf *m = mbufalloc(1518-(n+1));
  struct proc *pr = myproc();

  copyin(pr->pagetable, m->head, addr, n);

  mbufput(m, n);
  if (s->stype == SOCK_TCP || s->stype == SOCK_TCP_LISTEN) {
    // TODO 
    if (s->scb != 0)
      net_tx_tcp(s->scb, m, 0);
  } else {
    net_tx_udp(m, s->raddr, s->sport, s->dport);
  }
  return n;
}

int
sys_sockrecv(struct file *f, uint64 addr, int n)
{
  struct sock *sock = f->sock;
  struct mbufq *rxq = &sock->rxq;
  struct proc *pr = myproc();

  struct mbuf *m = mbufq_pophead(rxq);
  // TODO fix busy wait
  while (m == 0x0) {
    m = mbufq_pophead(rxq);
  }
  copyout(pr->pagetable, addr, m->head, n);
  mbuffree(m);
  return n;
}


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
#include "sys/syscall.h"
#include "sys/sysnet.h"

extern struct sock_cb_entry scb_table[SOCK_CB_LEN];
uint8 sport_table[SPORT_NUM];

struct spinlock sport_lock;
uint16 current_sport = START_OF_SPORT;

uint16 get_new_sport() {
  int islooped = 0;
  acquire(&sport_lock);
  while (islooped < 2) {
    if ((sport_table[current_sport/8] & 0xff) < 0xff) {
      for (int i = 0; i < 8; i++) {
        if ((sport_table[current_sport/8] & (1 << i)) == 0) {
          sport_table[current_sport/8] |= (1 << i);
          release(&sport_lock);
          return current_sport + i;
        }
      }
    }
    if (current_sport+8 >= MAX_SPORT) {
      current_sport = START_OF_SPORT;
      islooped += 1;
    } else {
      current_sport += 8;
    }
  }
  release(&sport_lock);
  panic("[get_new_sport] sport is full\n");
  return -1;
}

void release_sport(uint16 sport) {
  acquire(&sport_lock);
  if (((sport_table[sport/8]) & (1 << (sport % 8))) >= 1)
    sport_table[sport/8] ^= 1 << (sport % 8);
  else
    panic("sport didn't use");
  release(&sport_lock);
}

void
sockinit(void)
{
  initlock(&sport_lock, "sportlock");
  memset(scb_table, 0, sizeof(scb_table));
  for (int i = 0; i < SOCK_CB_LEN; i++) {
    scb_table[i].head = 0;
    initlock(&scb_table[i].lock, "scb entry lock");
  }
}

int
sockalloc(struct file **f, int socktype)
{
  struct sock_cb *scb;

  scb = 0;
  *f = 0;
  if ((*f = filealloc()) == 0)
    goto bad;
  if ((scb = (struct sock_cb*)bd_alloc(sizeof(*scb))) == 0)
    goto bad;

  // initialize objects
  scb->raddr = 0;
  scb->sport = 0;
  scb->dport = 0;
  scb->socktype = socktype;

  initlock(&scb->lock, "scb lock");
  mbufq_init(&scb->rxq);
  (*f)->type = FD_SOCK;
  (*f)->readable = 1;
  (*f)->writable = 1;
  (*f)->scb = scb;

  return 0;

bad:
  if (scb)
    kfree((char*)scb);
  if (*f)
    fileclose(*f);
  return -1;
}

void sockfree(struct sock_cb *scb) {
  bd_free((char*)scb);
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
  int socktype;

  if(
    argint(0, (int *)&socktype) < 0
  )
    return -1;

  int fd;
  if(sockalloc(&f, socktype) != 0 || (fd = fdalloc(f)) < 0)
    return -1;

  return fd;
}

uint64 sys_socklisten() {
  struct file *f;
  struct sock_cb *scb;
  uint16 sport;

  if (argfd(0, 0, &f) || argint(1, (int *)&sport) < 0) {
    return -1;
  }

  scb = f->scb;
  // file doesn*t equal socket or socket close
  if (f->type !=  FD_SOCK || scb == 0) {
    return -1;
  }

  if (scb->socktype == SOCK_TCP) {
    
  } else {

  }

  return 0;
}

uint64 sys_sockconnect() {
  struct file *f;
  struct sock_cb *scb;
  uint16 sport;

  if (argfd(0, 0, &f) || argint(1, (int *)&sport) < 0) {
    return -1;
  }

  scb = f->scb;
  // file doesn*t equal socket or socket close
  if (f->type !=  FD_SOCK || scb == 0) {
    return -1;
  } 

  return 0;
}

// UDP only now
int
socksend(struct file *f, uint64 addr, int n)
{
  // TODO split data
  struct sock_cb *scb = f->scb;
  struct mbuf *m = mbufalloc(1518-(n+1));
  struct proc *pr = myproc();

  copyin(pr->pagetable, m->head, addr, n);

  mbufput(m, n);
  if (scb == 0) {
    printf("scb is null\n");
    return -1;
  }
  if (scb->socktype == SOCK_TCP) {
    // TODO 
    net_tx_tcp(scb, m, 0);
  } else {
    net_tx_udp(m, scb->raddr, scb->sport, scb->dport);
  }
  return n;
}

int
sockrecv(struct file *f, uint64 addr, int n)
{
  struct sock_cb *scb = f->scb;
  struct mbufq *rxq = &scb->rxq;
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


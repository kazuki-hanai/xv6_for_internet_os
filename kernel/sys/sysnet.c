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

extern struct sock_cb_entry tcp_scb_table[SOCK_CB_LEN];
extern struct sock_cb_entry udp_scb_table[SOCK_CB_LEN];

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

uint16 get_specified_sport(uint16 sport) {
  uint16 res = -1;
  acquire(&sport_lock);
  if ((sport_table[sport/8] & (1 << (sport % 8))) == 0) {
    sport_table[sport/8] |= (1 << (sport % 8));
    res = sport;
  }
  release(&sport_lock);
  return res;
}

void release_sport(uint16 sport) {
  acquire(&sport_lock);
  if (((sport_table[sport/8]) & (1 << (sport % 8))) >= 1)
    sport_table[sport/8] ^= 1 << (sport % 8);
  release(&sport_lock);
}

void
sysnet_init(void)
{
  initlock(&sport_lock, "sportlock");
  memset(tcp_scb_table, 0, sizeof(tcp_scb_table));
  memset(udp_scb_table, 0, sizeof(udp_scb_table));
  for (int i = 0; i < SOCK_CB_LEN; i++) {
    tcp_scb_table[i].head = 0;
    udp_scb_table[i].head = 0;
    initlock(&tcp_scb_table[i].lock, "tcp scb entry lock");
    initlock(&udp_scb_table[i].lock, "udp scb entry lock");
  }
}

int
socket_alloc(struct file **f, int socktype)
{
  struct sock_cb *scb;

  scb = 0;
  *f = 0;
  if ((*f = filealloc()) == 0)
    goto bad;

  // initialize objects
  scb = init_sock_cb(0, 0, 0, socktype);
  (*f)->type = FD_SOCK;
  (*f)->readable = 1;
  (*f)->writable = 1;
  (*f)->scb = scb;

  return 0;
bad:
  if (*f)
    fileclose(*f);
  if (scb)
    sock_cb_free(scb);
  return -1;
}

void sock_cb_free(struct sock_cb *scb) {
  release_sport(scb->sport);
  bd_free((char*)scb);
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
  if(socket_alloc(&f, socktype) != 0 || (fd = fdalloc(f)) < 0)
    return -1;

  return fd;
}

uint64 sys_socklisten() {
  int fd;
  struct file *f;
  struct sock_cb *scb;
  uint16 sport;

  if (argint(1, (int *)&sport) < 0 || argfd(0, &fd, &f) < 0 ) {
    return -1;
  }

  scb = f->scb;
  // file doesn*t equal socket or socket close
  if (f->type !=  FD_SOCK || scb == 0) {
    return -1;
  }
  // port already used
  if (get_specified_sport(sport) < 0) {
    return -1;
  }
  scb->sport = sport;

  if (scb->socktype == SOCK_TCP) {
    add_sock_cb(tcp_scb_table, scb);
    if (tcp_listen(scb) < 0) {
      return -1;
    }
  } else {
    add_sock_cb(udp_scb_table, scb);
  }

  return 0;
}

uint64 sys_sockconnect() {
  struct file *f;
  struct sock_cb *scb;
  uint32 raddr;
  uint16 dport;

  if (argfd(0, 0, &f) < 0 || argint(1, (int *)&raddr) < 0 || argint(2, (int *)&dport) < 0) {
    return -1;
  }

  scb = f->scb;
  // file doesn*t equal socket or socket close
  if (f->type !=  FD_SOCK || scb == 0) {
    return -1;
  } 
  scb->raddr = raddr;
  scb->sport = get_new_sport();
  scb->dport = dport;

  if (scb->socktype == SOCK_TCP) {
    add_sock_cb(tcp_scb_table, scb);
    if (tcp_connect(scb) < 0) {
      return -1;
    }
  } else {
    add_sock_cb(udp_scb_table, scb);
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
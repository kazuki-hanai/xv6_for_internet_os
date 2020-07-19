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
#include "net/udp.h"
#include "net/ethernet.h"
#include "net/arp.h"
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
    if ((sport_table[(current_sport - START_OF_SPORT)/8] & 0xff) < 0xff) {
      for (int i = 0; i < 8; i++) {
        if ((sport_table[(current_sport - START_OF_SPORT)/8] & (1 << i)) == 0) {
          sport_table[(current_sport - START_OF_SPORT)/8] |= (1 << i);
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
  if ((sport_table[(sport - START_OF_SPORT)/8] & (1 << ((sport - START_OF_SPORT) % 8))) == 0) {
    sport_table[(sport - START_OF_SPORT)/8] |= (1 << ((sport - START_OF_SPORT) % 8));
    res = sport;
  }
  release(&sport_lock);
  return res;
}

void release_sport(uint16 sport) {
  acquire(&sport_lock);
  if (((sport_table[(sport - START_OF_SPORT)/8]) & (1 << ((sport - START_OF_SPORT) % 8))) >= 1)
    sport_table[(sport - START_OF_SPORT)/8] ^= 1 << ((sport - START_OF_SPORT) % 8);
  release(&sport_lock);
}

void
sysnet_init(void)
{
  initlock(&sport_lock, "sportlock");
  printf("sport: %d\n", sport_table[(current_sport - START_OF_SPORT)/8]);
  memset(sport_table, 0, sizeof(sport_table));
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
  scb = init_sock_cb(*f, 0, 0, 0, socktype);
  (*f)->type = FD_SOCK;
  (*f)->readable = 1;
  (*f)->writable = 1;
  (*f)->scb = scb;

  return 0;
bad:
  if (*f)
    fileclose(*f);
  if (scb)
    free_sock_cb(scb);
  return -1;
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

uint64 sys_socklisten_core(struct sock_cb *scb, uint16 sport) {
  // port already used
  if (get_specified_sport(sport) < 0) {
    return -1;
  }
  scb->sport = sport;

  if (scb->socktype == SOCK_TCP) {
    add_sock_cb(scb);
    if (tcp_listen(scb) < 0) {
      return -1;
    }
  } else {
    add_sock_cb(scb);
  }

  return 0;
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
  
  return sys_socklisten_core(scb, sport);
}

uint64 sys_sockconnect_core(struct sock_cb *scb, uint32 raddr, uint16 dport) {
  scb->raddr = raddr;
  scb->sport = get_new_sport();
  scb->dport = dport;

  if (scb->socktype == SOCK_TCP) {
    add_sock_cb(scb);
    if (tcp_connect(scb) < 0) {
      return -1;
    }
  } else {
    add_sock_cb(scb);
  }

  return 0;
}

uint64 sys_sockconnect() {
  struct file *f;
  struct sock_cb *scb;
  uint32 raddr;
  uint16 dport;

  if (argint(2, (int *)&dport) < 0 || argint(1, (int *)&raddr) < 0 || argfd(0, 0, &f) < 0) {
    return -1;
  }

  scb = f->scb;
  // file doesn*t equal socket or socket close
  if (f->type !=  FD_SOCK || scb == 0) {
    return -1;
  }

  return sys_sockconnect_core(scb, raddr, dport); 
}

// UDP only now
int
socksend(struct file *f, uint64 addr, int n)
{
  struct sock_cb *scb = f->scb;
  struct proc *pr = myproc();

  int bufsize = 0;
  int res = 0;
  while (n > 0) {
    struct mbuf *m = mbufalloc(ETH_MAX_SIZE);
    if (m == 0) {
      return -1;
    }
    if (scb->socktype == SOCK_TCP) {
      bufsize = n < TCP_MAX_DATA ? n : TCP_MAX_DATA;
    } else {
      bufsize = n < UDP_MAX_DATA ? n : UDP_MAX_DATA;
    }
    mbufpush(m, bufsize);
    copyin(pr->pagetable, m->head, addr, bufsize);

    if (scb == 0) {
      printf("scb is null\n");
      return -1;
    }
    if (scb->socktype == SOCK_TCP) {
      int flg = n < TCP_MAX_DATA ? TCP_FLG_PSH | TCP_FLG_ACK : TCP_FLG_ACK;
      if (tcp_send(scb, m, flg) == -1) {
        mbuffree(m);
        printf("[socksend] tcp_send error\n");
        return -1;
      }
    } else {
      udp_send(m, scb->raddr, scb->sport, scb->dport);
    }
    n -= bufsize;
    addr += bufsize;
    res += bufsize;
  }
  return res;
}

int
sockrecv(struct file *f, uint64 addr, int n)
{
  struct sock_cb *scb = f->scb;
  struct proc *pr = myproc();

  struct mbuf *m = 0;
  int res = 0;
  // TODO fix busy wait
  // TODO tcp push check
  
  if (scb->socktype == SOCK_TCP) {
    while (1) {
      m = pop_from_scb_rxq(scb);
      if (m == 0)
        continue;
      
      int acceptable_size = n > m->len ? m->len : n;
      
      copyout(pr->pagetable, addr, m->head, acceptable_size);
      addr += acceptable_size;
      scb->rcv.wnd += acceptable_size;
      res += acceptable_size;

      if (n > m->len) {
        mbuffree(m);
        if (TCP_FLG_ISSET(m->tcphdr->flg, TCP_FLG_PSH)) {
          break;
        }
      } else {
        mbufpull(m, acceptable_size);
        mbufq_pushhead(&scb->rxq, m);
        break;
      }
      n -= acceptable_size;
    }
  } else {
    // busy-wait
    while (m == 0x0) {
      m = pop_from_scb_rxq(scb);
    }
    int acceptable_size = n > m->len ? m->len : n;

    copyout(pr->pagetable, addr, m->head, acceptable_size);
    addr += acceptable_size;

    if (m->len > n-1) {
      mbufpull(m, n-1);
      mbufq_pushhead(&scb->rxq, m);
    } else {
      mbuffree(m);
    }
    res = acceptable_size;
  }

  return res;
}

void sockclose(struct file *f)
{
  struct sock_cb *scb = f->scb;

  if (!scb) {
    panic("[sockclose] scb is already freed");
  }
  if (scb->socktype == SOCK_TCP) {
    if (tcp_close(scb) == 0) {
      free_sock_cb(f->scb);
    }
  } else {
    free_sock_cb(f->scb);
  }
}
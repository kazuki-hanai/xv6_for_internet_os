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
#include "net/sock_cb.h"
#include "net/socket.h"
#include "sys/syscall.h"

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
  if (sport > MAX_SPORT)
    return;
  acquire(&sport_lock);
  if (((sport_table[(sport - START_OF_SPORT)/8]) & (1 << ((sport - START_OF_SPORT) % 8))) >= 1)
    sport_table[(sport - START_OF_SPORT)/8] ^= 1 << ((sport - START_OF_SPORT) % 8);
  release(&sport_lock);
}

void socket_init() {
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

struct sock_cb* sockalloc(int socktype) {
  return alloc_sock_cb(0, 0, 0, 0, socktype);
}

void sockfree(struct sock_cb *scb) {
  free_sock_cb(scb);
}

uint64 socklisten(struct sock_cb *scb, uint16 sport) {
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

uint64 sockconnect(struct sock_cb *scb, uint32 raddr, uint16 dport) {
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

int socksend(struct sock_cb *scb, uint64 addr, int n, int is_copyin) {
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

    if (is_copyin)
      copyin(pr->pagetable, m->head, addr, bufsize);
    else
      memmove(m->head, (void *)addr, bufsize);

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

int sockrecv(struct sock_cb *scb, uint64 addr, int n, int is_copyout) {
  struct proc *pr = myproc();

  struct mbuf *m = 0;
  int res = 0;
  // TODO fix busy wait
  // TODO tcp push check
  
  if (scb->socktype == SOCK_TCP) {
    uint16 sport = scb->sport;
    while (1) {
      scb = get_sock_cb(tcp_scb_table, sport);
      if (scb == 0 || scb->state == SOCK_CB_CLOSE_WAIT)
        return -1;
      m = pop_from_scb_rxq(scb);

      if (m == 0) {
        continue;
      }
      
      int acceptable_size = n > m->len ? m->len : n;
      
      if (is_copyout)
        copyout(pr->pagetable, addr, m->head, acceptable_size);
      else
        memmove((void *)addr, m->head, acceptable_size);
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

    if (is_copyout)
      copyout(pr->pagetable, addr, m->head, acceptable_size);
    else
      memmove((void *)addr, m->head, acceptable_size);
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

void sockclose(struct sock_cb *scb)
{
  if (!scb) {
    panic("[sockclose] scb is already freed");
  }
  if (scb->socktype == SOCK_TCP) {
    if (tcp_close(scb) == 0) {
      free_sock_cb(scb);
    }
  } else {
    free_sock_cb(scb);
  }
}

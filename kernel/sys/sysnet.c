//
// network system calls.
//
#include "types.h"
#include "defs.h"
#include "file.h"
#include "net/socket.h"

uint64_t
sys_socket(void)
{
  struct file *f;
  struct sock_cb *scb;
  int socktype;

  if(
    argint(0, (int *)&socktype) < 0
  )
    return -1;

  if ((f = filealloc()) == 0)
    goto bad;
  if ((scb = sockalloc(socktype)) == 0)
    goto bad;
  f->type = FD_SOCK;
  f->readable = 1;
  f->writable = 1;
  f->scb = scb;

  int fd;
  if((fd = fdalloc(f)) < 0)
    return -1;

  return fd;

bad:
  if (f)
    fileclose(f);
  if (scb)
    free_sock_cb(scb);
  return -1;
}

uint64_t sys_socklisten() {
  int fd;
  struct file *f;
  struct sock_cb *scb;
  uint16_t sport;

  if (argint(1, (int *)&sport) < 0 || argfd(0, &fd, &f) < 0 ) {
    return -1;
  }

  scb = f->scb;
  // file doesn*t equal socket or socket close
  if (f->type !=  FD_SOCK || scb == 0) {
    return -1;
  }
  
  return socklisten(scb, sport);
}


uint64_t sys_sockconnect() {
  struct file *f;
  struct sock_cb *scb;
  uint32_t raddr;
  uint16_t dport;

  if (argint(2, (int *)&dport) < 0 || argint(1, (int *)&raddr) < 0 || argfd(0, 0, &f) < 0) {
    return -1;
  }

  scb = f->scb;
  // file doesn*t equal socket or socket close
  if (f->type !=  FD_SOCK || scb == 0) {
    return -1;
  }

  return sockconnect(scb, raddr, dport); 
}

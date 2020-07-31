#include "types.h"
#include "arch/riscv.h"
#include "defs.h"
#include "param.h"
#include "net/byteorder.h"
#include "net/socket.h"
#include "net/styx2000.h"
#include "fcall.h"

struct styx2000_req* styx2000_allocreq() {
  struct styx2000_req *req;
  req = bd_alloc(sizeof(*req));
  if (req == 0) {
    panic("[styx2000_allocreq] could not allocate");
  }
  return req;
}

void styx2000_freereq(struct styx2000_req *req) {
  bd_free(req);
}

int styx2000_sendreq(struct styx2000_server *srv, struct styx2000_req *req) {
  if (socksend(srv->scb, (uint64)srv->wbuf, req->ofcall.size, 0) <= 0) {
    return -1;
  }
  return 0;
}

struct styx2000_req* styx2000_recvreq(struct styx2000_server *srv) {
  int rsize;
  if ((rsize = sockrecv(srv->scb, (uint64)srv->rbuf, STYX2000_MAXMSGLEN, 0)) == -1) {
    return 0;
  }
  struct styx2000_req *req;
  if ((req = styx2000_parsefcall(srv->rbuf, rsize)) == 0) {
    return 0;
  }
  return req;
}

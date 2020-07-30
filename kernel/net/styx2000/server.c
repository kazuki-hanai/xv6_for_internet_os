#include "types.h"
#include "arch/riscv.h"
#include "defs.h"
#include "param.h"
#include "net/byteorder.h"
#include "net/socket.h"
#include "net/styx2000.h"

static int start_server(struct styx2000_server *srv) {
  srv->msize = STYX2000_MAXMSGLEN;
  srv->wbuf = bd_alloc(srv->msize);
  srv->scb = sockalloc(SOCK_TCP);
  if (socklisten(srv->scb, STYX2000_PORT) == -1) {
    return -1;
  }
  return 0;
}

static void stop_server(struct styx2000_server *srv) {
  sockclose(srv->scb);
  bd_free(srv->wbuf);
  srv->scb = 0;
}

static int sendpacket(struct styx2000_server *srv, struct styx2000_req *req) {
  if (socksend(srv->scb, (uint64)srv->wbuf, req->ofcall.size, 0) <= 0) {
    return -1;
  }
  return 0;
}

static struct styx2000_req* getreq(struct styx2000_server *srv) {
  uint8 rbuf[2048];
  int rsize;
  if ((rsize = sockrecv(srv->scb, (uint64)rbuf, sizeof(rbuf), 0)) == -1) {
    return 0;
  }
  struct styx2000_req *req;
  if ((req = styx2000_parsefcall(rbuf, rsize)) == 0) {
    return 0;
  }
  return req;
}

static void initserver(struct styx2000_server *srv) {
  srv->msize = STYX2000_MAXMSGLEN;
  srv->start = start_server;
  srv->stop = stop_server;
  srv->send = sendpacket;
}

int styx2000_serve() {
  struct styx2000_server srv;
  initserver(&srv);

  if (srv.start(&srv) == -1) {
    goto fail;
  }

  struct styx2000_req *req;
  while ((req = getreq(&srv)) != 0) {
    switch (req->ifcall.type) {
      case STYX2000_TVERSION:
        if (styx2000_rversion(&srv, req) == -1) {
          goto fail;
        }
        break;
      case STYX2000_RVERSION:
        goto fail;
        break;
      case STYX2000_TAUTH:
        return 0;
        break;
      case STYX2000_RAUTH:
        return 0;
        break;
      case STYX2000_TATTACH:
        break;
      case STYX2000_RATTACH:
        break;
      case STYX2000_RERROR:
        break;
      case STYX2000_TFLUSH:
        break;
      case STYX2000_RFLUSH:
        break;
      case STYX2000_TWALK:
        break;
      case STYX2000_RWALK:
        break;
      case STYX2000_TOPEN:
        break;
      case STYX2000_ROPEN:
        break;
      case STYX2000_TCREATE:
        break;
      case STYX2000_RCREATE:
        break;
      case STYX2000_TREAD:
        break;
      case STYX2000_RREAD:
        break;
      case STYX2000_TWRITE:
        break;
      case STYX2000_RWRITE:
        break;
      case STYX2000_TCLUNK:
        break;
      case STYX2000_RCLUNK:
        break;
      case STYX2000_TREMOVE:
        break;
      case STYX2000_RREMOVE:
        break;
      case STYX2000_TSTAT:
        break;
      case STYX2000_RSTAT:
        break;
      case STYX2000_TWSTAT:
        break;
      case STYX2000_RWSTAT:
        break;
    }
  }
  srv.stop(&srv);
  return 0;

fail:
  srv.stop(&srv);
  return -1;
}

#include "user.h"
#include "net/byteorder.h"
#include "net/socket.h"
#include "styx2000.h"
#include "fcall.h"

struct styx2000_req* styx2000_allocreq() {
  struct styx2000_req *req;
  req = malloc(sizeof(*req));
  if (req == 0) {
    printf("[styx2000_allocreq] could not allocate");
    return 0;
  }
  memset(req, 0, sizeof *req);
  return req;
}

void styx2000_freereq(struct styx2000_req *req) {
  if (req->ofcall.type == STYX2000_RREAD) {
    free(req->ofcall.data);
  } else if (req->ofcall.type == STYX2000_RSTAT) {
    free(req->ofcall.stat);
  } else if (req->ifcall.type == STYX2000_TWRITE) {
    free(req->ifcall.data);
  }
  free(req);
}

int styx2000_sendreq(struct styx2000_conn *conn, struct styx2000_req *req) {
  if (write(conn->sockfd, conn->wbuf, req->ofcall.size) <= 0) {
    return -1;
  }
  return 0;
}

struct styx2000_req* styx2000_recvreq(struct styx2000_conn *conn) {
  int rsize;
  if ((rsize = read(conn->sockfd, conn->rbuf, STYX2000_MAXMSGLEN)) == -1) {
    return 0;
  }
  struct styx2000_req *req;
  if ((req = styx2000_parsefcall(conn->rbuf, rsize)) == 0) {
    return 0;
  }
  return req;
}

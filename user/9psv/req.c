#include "user.h"
#include "net/byteorder.h"
#include "net/socket.h"
#include "p9.h"

struct p9_req* p9_allocreq() {
  struct p9_req *req;
  req = p9malloc(sizeof(*req));
  memset(req, 0, sizeof *req);
  return req;
}

void p9_freereq(struct p9_req *req) {
  if (req->ofcall.type == P9_RREAD) {
    free(req->ofcall.data);
  } else if (req->ofcall.type == P9_RSTAT) {
    p9_freestat(req->ofcall.stat);
  } else if (req->ifcall.type == P9_TWRITE) {
    free(req->ifcall.data);
  }
  free(req);
}

int p9_sendreq(struct p9_conn *conn, struct p9_req *req) {
  if (write(conn->sockfd, conn->wbuf, req->ofcall.size) <= 0) {
    return -1;
  }
  return 0;
}

struct p9_req* p9_recvreq(struct p9_conn *conn) {
  int rsize;
  if ((rsize = read(conn->sockfd, conn->rbuf, P9_MAXMSGLEN)) == -1) {
    return 0;
  }

  int size = GBIT32(conn->rbuf);
  while(rsize != size) {
    if ((rsize += read(conn->sockfd, conn->rbuf+rsize, P9_MAXMSGLEN-rsize)) <= 0) {
      return 0;
    }
  }

  struct p9_req *req;
  if ((req = parsefcall(conn->rbuf, rsize)) == 0) {
    return 0;
  }
  return req;
}

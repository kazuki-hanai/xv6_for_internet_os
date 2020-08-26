#include "user.h"
#include "net/byteorder.h"
#include "net/socket.h"
#include "p9.h"

struct p9_req* p9_allocreq() {
  struct p9_req *req;
  req = malloc(sizeof(*req));
  if (req == 0) {
    printf("[p9_allocreq] could not allocate");
    return 0;
  }
  memset(req, 0, sizeof *req);
  return req;
}

void p9_freereq(struct p9_req *req) {
  if (req->ofcall.type == P9_RREAD) {
    free(req->ofcall.data);
  } else if (req->ofcall.type == P9_RSTAT) {
    free(req->ofcall.stat);
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
  struct p9_req *req;
  if ((req = p9_parsefcall(conn->rbuf, rsize)) == 0) {
    return 0;
  }
  return req;
}

static uint8_t* parse_hdr(struct p9_fcall *fcall, uint8_t* buf) {
  fcall->size = GBIT32(buf);
  buf += 4;
  fcall->type = GBIT8(buf);
  buf += 1;
  fcall->tag = GBIT16(buf);
  buf += 2;
  return buf;
}
struct p9_req* p9_parsefcall(uint8_t* buf, int size) {
  if (buf == 0) {
    return 0;
  }
  if (size < P9_HDR_SIZE) {
    return 0;
  }

  struct p9_req *req = p9_allocreq();
  if (req == 0) {
    return 0;
  }
  memset(req, 0, sizeof *req);
  struct p9_fcall *ifcall = &req->ifcall;

  buf = parse_hdr(ifcall, buf);
  int mlen = ifcall->size - P9_HDR_SIZE;

  switch (ifcall->type) {
    case P9_TVERSION:
      buf = p9_parse_tversion(ifcall, buf, mlen);
      break;
    case P9_TAUTH:
      break;
    case P9_TATTACH:
      buf = p9_parse_tattach(ifcall, buf, mlen);
      break;
    case P9_TWALK:
      buf = p9_parse_twalk(ifcall, buf, mlen);
      break;
    case P9_TOPEN:
      buf = p9_parse_topen(ifcall, buf, mlen);
      break;
    case P9_TFLUSH:
      break;
    case P9_TCREATE:
      buf = p9_parse_tcreate(ifcall, buf, mlen);
      break;
    case P9_TREAD:
      buf = p9_parse_tread(ifcall, buf, mlen);
      break;
    case P9_TWRITE:
      buf = p9_parse_twrite(ifcall, buf, mlen);
      break;
    case P9_TCLUNK:
      buf = p9_parse_tclunk(ifcall, buf, mlen);
      break;
    case P9_TREMOVE:
      buf = p9_parse_tremove(ifcall, buf, mlen);
      break;
    case P9_TSTAT:
      buf = p9_parse_tstat(ifcall, buf, mlen);
      break;
    case P9_TWSTAT:
      break;
    default:
      goto fail;
  }
  if (buf == 0) {
    goto fail;
  }
  return req;

fail:
  if (req)
    p9_freereq(req);
  return 0;
}

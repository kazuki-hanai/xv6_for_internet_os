#include "user.h"
#include "types.h"
#include "arch/riscv.h"
#include "param.h"
#include "net/styx2000.h"
#include "net/styx2000util.h"
#include "net/byteorder.h"
#include "fcall.h"

static uint8* parse_hdr(struct styx2000_fcall *fcall, uint8* buf) {
  fcall->size = GBIT32(buf);
  buf += 4;
  fcall->type = GBIT8(buf);
  buf += 1;
  fcall->tag = GBIT16(buf);
  buf += 2;
  return buf;
}

struct styx2000_req* styx2000_parsefcall(uint8* buf, int size) {
  if (buf == 0) {
    return 0;
  }
  if (size < STYX2000_HDR_SIZE) {
    return 0;
  }

  struct styx2000_req *req = styx2000_allocreq();
  if (req == 0) {
    return 0;
  }
  memset(req, 0, sizeof *req);
  struct styx2000_fcall *ifcall = &req->ifcall;

  buf = parse_hdr(ifcall, buf);
  int mlen = ifcall->size - STYX2000_HDR_SIZE;

  switch (ifcall->type) {
    case STYX2000_TVERSION:
      buf = styx2000_parse_tversion(ifcall, buf, mlen);
      break;
    case STYX2000_RVERSION:
      break;
    case STYX2000_TAUTH:
      return 0;
      break;
    case STYX2000_RAUTH:
      return 0;
      break;
    case STYX2000_TATTACH:
      buf = styx2000_parse_tattach(ifcall, buf, mlen);
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

  if (buf == 0) {
    goto fail;
  }

  return req;

fail:
  if (req)
    styx2000_freereq(req);
  return 0;
}

static int composefcall(struct styx2000_req *req, uint8* buf, int size) {
  struct styx2000_fcall *f = &req->ofcall;
  PBIT32(buf, f->size);
  buf += 4;
  PBIT8(buf, f->type);
  buf += 1;
  PBIT16(buf, f->tag);
  buf += 2;

  switch (f->type) {
    case STYX2000_TVERSION:
      break;
    case STYX2000_RVERSION:
      if (styx2000_compose_rversion(req, buf) == -1) {
        return -1;
      }
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
      if (styx2000_compose_rattach(req, buf) == -1) {
        return -1;
      }
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
  return 0;
}

int styx2000_respond(struct styx2000_server *srv, struct styx2000_req *req) {
  req->ofcall.type = req->ifcall.type+1;
  req->ofcall.size = getfcallsize(&req->ofcall);
  req->ofcall.tag = req->ifcall.tag;

  // TODO error processing

  if (composefcall(req, srv->wbuf, srv->msize) == -1 ) {
    return -1;
  }

  if (srv->send(srv, req) == -1) {
    return -1;
  }
  return 0;
}

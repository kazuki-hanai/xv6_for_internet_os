#include "user.h"
#include "types.h"
#include "arch/riscv.h"
#include "param.h"
#include "net/styx2000.h"
#include "net/styx2000util.h"
#include "net/byteorder.h"
#include "fcall.h"

uint8* styx2000_parse_tversion(struct styx2000_fcall *fcall, uint8* buf, int len) {
  if (fcall->tag != STYX2000_NOTAG) {
    return 0;
  }
  uint8 *ep = buf + len;
  fcall->msize = GBIT32(buf);
  buf += 4;
  buf = gstring(buf, ep, &fcall->version);
  if (buf == 0) {
    return 0;
  }
  return buf;
}

int styx2000_tversion(struct styx2000_server *srv, struct styx2000_req *req) {
  if (strncmp(req->ifcall.version, "9P2000", 6) != 0) {
    req->ofcall.version = "unknown";
    styx2000_respond(srv, req);
    return -1;
  }
  req->ofcall.version = "9P2000";
  req->ofcall.msize = req->ifcall.msize;
  if (styx2000_respond(srv, req) == -1) {
    return -1;
  }
  return 0;
}

int styx2000_compose_rversion(struct styx2000_req *req, uint8* buf) {
  struct styx2000_fcall *f = &req->ofcall;
  PBIT32(buf, f->msize);
  buf += BIT32SZ;
  buf = pstring(buf, f->version);
  return 0;
}

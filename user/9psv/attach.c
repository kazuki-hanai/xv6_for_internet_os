#include "user.h"
#include "param.h"
#include "styx2000.h"
#include "styx2000util.h"
#include "net/byteorder.h"
#include "fcall.h"

uint8* styx2000_parse_tattach(struct styx2000_fcall *fcall, uint8* buf, int len) {
  uint8 *ep = buf + len;
  fcall->fid = GBIT32(buf);
  buf += 4;
  fcall->afid = GBIT32(buf);
  buf += 4;
  buf = gstring(buf, ep, &fcall->uname);
  if (buf == 0) {
    return 0;
  }
  buf = gstring(buf, ep, &fcall->aname);
  if (buf == 0) {
    return 0;
  }
  return buf;
}

int styx2000_compose_rattach(struct styx2000_req *req, uint8* buf) {
  struct styx2000_fcall *f = &req->ofcall;
  PBIT8(buf, f->qid.type);
  buf += BIT8SZ;
  PBIT32(buf, f->qid.vers);
  buf += BIT32SZ;
  PBIT64(buf, f->qid.path);
  buf += BIT64SZ;
  return 0;
}

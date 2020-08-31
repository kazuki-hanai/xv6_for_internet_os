#include "user.h"
#include "../p9.h"
#include "net/byteorder.h"

uint8_t* p9_parse_tcreate(struct p9_fcall *f, uint8_t* buf, int len) {
  uint8_t *ep = buf + len;
  f->fid = GBIT32(buf);
  buf += BIT32SZ;
  buf = p9_gstring(buf, ep, &f->name);
  f->perm = GBIT32(buf);
  buf += BIT32SZ;
  f->mode = GBIT8(buf);
  buf += BIT8SZ;
  return buf;
}

int p9_compose_rcreate(struct p9_fcall *f, uint8_t* buf) {
  PBIT8(buf, f->qid.type);
  buf += BIT8SZ;
  PBIT32(buf, f->qid.vers);
  buf += BIT32SZ;
  PBIT64(buf, f->qid.path);
  buf += BIT64SZ;
  PBIT32(buf, f->iounit);
  buf += BIT32SZ;
  return 0;
}

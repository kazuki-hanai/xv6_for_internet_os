#include "user.h"
#include "p9.h"
#include "net/byteorder.h"

uint8_t* p9_parse_tattach(struct p9_fcall *fcall, uint8_t* buf, int len) {
  uint8_t *ep = buf + len;
  fcall->fid = GBIT32(buf);
  buf += 4;
  fcall->afid = GBIT32(buf);
  buf += 4;
  buf = p9_gstring(buf, ep, &fcall->uname);
  if (buf == 0) {
    return 0;
  }
  buf = p9_gstring(buf, ep, &fcall->aname);
  if (buf == 0) {
    return 0;
  }
  return buf;
}

int p9_compose_rattach(struct p9_fcall *f, uint8_t* buf) {
  PBIT8(buf, f->qid->type);
  buf += BIT8SZ;
  PBIT32(buf, f->qid->vers);
  buf += BIT32SZ;
  PBIT64(buf, f->qid->path);
  buf += BIT64SZ;
  return 0;
}

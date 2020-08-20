#include "user.h"
#include "p9.h"
#include "net/byteorder.h"

uint8_t* p9_parse_tversion(struct p9_fcall *fcall, uint8_t* buf, int len) {
  if (fcall->tag != P9_NOTAG) {
    return 0;
  }
  uint8_t *ep = buf + len;
  fcall->msize = GBIT32(buf);
  buf += 4;
  buf = p9_gstring(buf, ep, &fcall->version);
  if (buf == 0) {
    return 0;
  }
  return buf;
}

int p9_compose_rversion(struct p9_fcall *f, uint8_t* buf) {
  PBIT32(buf, f->msize);
  buf += BIT32SZ;
  buf = p9_pstring(buf, f->version);
  return 0;
}

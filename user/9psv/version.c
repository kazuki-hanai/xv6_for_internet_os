#include "user.h"
#include "types.h"
#include "styx2000.h"
#include "net/byteorder.h"
#include "fcall.h"

uint8* styx2000_parse_tversion(struct styx2000_fcall *fcall, uint8* buf, int len) {
  if (fcall->tag != STYX2000_NOTAG) {
    return 0;
  }
  uint8 *ep = buf + len;
  fcall->msize = GBIT32(buf);
  buf += 4;
  buf = styx2000_gstring(buf, ep, &fcall->version);
  if (buf == 0) {
    return 0;
  }
  return buf;
}

int styx2000_compose_rversion(struct styx2000_fcall *f, uint8* buf) {
  PBIT32(buf, f->msize);
  buf += BIT32SZ;
  buf = styx2000_pstring(buf, f->version);
  return 0;
}

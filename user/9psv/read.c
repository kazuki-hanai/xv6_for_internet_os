#include "user.h"
#include "param.h"
#include "styx2000.h"
#include "net/byteorder.h"
#include "fcall.h"

uint8* styx2000_parse_tread(struct styx2000_fcall *fcall, uint8* buf, int len) {
  fcall->fid = GBIT32(buf);
  buf += BIT32SZ;
  fcall->offset = GBIT64(buf);
  buf += BIT64SZ;
  fcall->count = GBIT32(buf);
  buf += BIT32SZ;
  return buf;
}

int styx2000_compose_rread(struct styx2000_fcall *f, uint8* buf) {
  PBIT32(buf, f->count);
  buf += BIT32SZ;
  for (int i = 0; i < f->count; i++) {
    PBIT8(buf, f->data[i]);
    buf += 1;
  }
  return 0;
}

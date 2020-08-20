#include "user.h"
#include "p9.h"
#include "net/byteorder.h"

uint8_t* p9_parse_tread(struct p9_fcall *fcall, uint8_t* buf, int len) {
  fcall->fid = GBIT32(buf);
  buf += BIT32SZ;
  fcall->offset = GBIT64(buf);
  buf += BIT64SZ;
  fcall->count = GBIT32(buf);
  buf += BIT32SZ;
  return buf;
}

int p9_compose_rread(struct p9_fcall *f, uint8_t* buf) {
  PBIT32(buf, f->count);
  buf += BIT32SZ;
  for (int i = 0; i < f->count; i++) {
    PBIT8(buf, f->data[i]);
    buf += 1;
  }
  return 0;
}

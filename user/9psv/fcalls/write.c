#include "user.h"
#include "../p9.h"
#include "net/byteorder.h"

uint8_t* p9_parse_twrite(struct p9_fcall *f, uint8_t* buf, int len) {
  f->fid = GBIT32(buf);
  buf += BIT32SZ;
  f->offset = GBIT64(buf);
  buf += BIT64SZ;
  f->count = GBIT32(buf);
  buf += BIT32SZ;
  f->count = f->count >= P9_MAXDATALEN ? P9_MAXDATALEN : f->count;
  f->data = p9malloc(f->count);
  for (int i = 0; i < f->count; i++) {
    f->data[i] = GBIT8(buf);
    buf += BIT8SZ;
  }
  return buf;
}

int p9_compose_rwrite(struct p9_fcall *f, uint8_t* buf) {
  PBIT32(buf, f->count);
  buf += BIT32SZ;
  return 0;
}

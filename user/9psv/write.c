#include "user.h"
#include "styx2000.h"
#include "net/byteorder.h"
#include "fcall.h"

uint8_t* styx2000_parse_twrite(struct styx2000_fcall *f, uint8_t* buf, int len) {
  f->fid = GBIT32(buf);
  buf += BIT32SZ;
  f->offset = GBIT64(buf);
  buf += BIT64SZ;
  f->count = GBIT32(buf);
  buf += BIT32SZ;
  f->count = f->count >= STYX2000_MAXMSGLEN ? 4000 : f->count;
  f->data = malloc(f->count);
  for (int i = 0; i < f->count; i++) {
    PBIT8(buf, f->data[i]);
    buf += 1;
  }
  return buf;
}

int styx2000_compose_rwrite(struct styx2000_fcall *f, uint8_t* buf) {
  PBIT32(buf, f->count);
  buf += BIT32SZ;
  return 0;
}

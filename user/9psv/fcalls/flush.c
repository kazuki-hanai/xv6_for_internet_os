#include "user.h"
#include "../p9.h"
#include "net/byteorder.h"

uint8_t* p9_parse_tflush(struct p9_fcall *f, uint8_t* buf, int len) {
  f->oldtag = GBIT16(buf);
  buf += BIT16SZ;
  return buf;
}

int p9_compose_rflush(struct p9_fcall *f, uint8_t* buf) {
  return 0;
}

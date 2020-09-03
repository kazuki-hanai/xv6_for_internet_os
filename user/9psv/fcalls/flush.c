#include "user.h"
#include "../p9.h"
#include "net/byteorder.h"

uint8_t* parse_tflush(struct p9_fcall *f, uint8_t* buf, int len) {
  f->oldtag = GBIT16(buf);
  buf += BIT16SZ;
  return buf;
}

int compose_rflush(struct p9_fcall *f, uint8_t* buf) {
  return 0;
}

int size_tflush(struct p9_fcall *f) {
  int n = 0;
  n += BIT16SZ;
  return n;
}

int size_rflush(struct p9_fcall *f) {
  int n = 0;
  return n;
}

void debug_tflush(struct p9_fcall* f) {
  printf("<= TFLUSH: \n");
}

void debug_rflush(struct p9_fcall* f) {
  printf("=> RFLUSH: \n");
}

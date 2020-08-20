#include "user.h"
#include "p9.h"
#include "net/byteorder.h"

int p9_compose_rlerror(struct p9_fcall *f, uint8_t* buf) {
  PBIT32(buf, f->ecode);
  return 0;
}

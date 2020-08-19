#include "user.h"
#include "param.h"
#include "styx2000.h"
#include "net/byteorder.h"
#include "fcall.h"

int styx2000_compose_rerror(struct styx2000_fcall *f, uint8_t* buf) {
  buf = styx2000_pstring(buf, f->ename);
  return 0;
}

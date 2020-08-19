#include "user.h"
#include "param.h"
#include "styx2000.h"
#include "net/byteorder.h"
#include "fcall.h"

uint8_t* styx2000_parse_tremove(struct styx2000_fcall *fcall, uint8_t* buf, int len) {
  fcall->fid = GBIT32(buf);
  buf += 4;
  return buf;
}

int styx2000_compose_rremove(struct styx2000_fcall *f, uint8_t* buf) {
  return 0;
}


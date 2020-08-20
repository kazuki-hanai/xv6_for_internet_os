#include "user.h"
#include "p9.h"
#include "net/byteorder.h"

uint8_t* p9_parse_tclunk(struct p9_fcall *fcall, uint8_t* buf, int len) {
  fcall->fid = GBIT32(buf);
  buf += 4;
  return buf;
}

int p9_compose_rclunk(struct p9_fcall *f, uint8_t* buf) {
  return 0;
}

#include "user.h"
#include "param.h"
#include "styx2000.h"
#include "net/byteorder.h"
#include "fcall.h"

uint8* styx2000_parse_tclunk(struct styx2000_fcall *fcall, uint8* buf, int len) {
  fcall->fid = GBIT32(buf);
  buf += 4;
  return buf;
}

int styx2000_compose_rclunk(struct styx2000_req *req, uint8* buf) {
  return 0;
}

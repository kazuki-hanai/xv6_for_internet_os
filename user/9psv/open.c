#include "user.h"
#include "param.h"
#include "styx2000.h"
#include "net/byteorder.h"
#include "fcall.h"

uint8* styx2000_parse_topen(struct styx2000_fcall *fcall, uint8* buf, int len) {
  fcall->fid = GBIT32(buf);
  buf += 4;
  fcall->mode = GBIT8(buf);
  buf += 1;
  return buf;
}

int styx2000_compose_ropen(struct styx2000_fcall *f, uint8* buf) {
  PBIT8(buf, f->qid->type);
  buf += BIT8SZ;
  PBIT32(buf, f->qid->vers);
  buf += BIT32SZ;
  PBIT64(buf, f->qid->path);
  buf += BIT64SZ;
  PBIT32(buf, f->iounit);
  buf += BIT32SZ;
  return 0;
}

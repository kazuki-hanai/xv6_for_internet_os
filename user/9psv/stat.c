#include "user.h"
#include "param.h"
#include "styx2000.h"
#include "net/byteorder.h"
#include "fcall.h"

uint8* styx2000_parse_tstat(struct styx2000_fcall *fcall, uint8* buf, int len) {
  fcall->fid = GBIT32(buf);
  buf += 4;
  return buf;
}

int styx2000_compose_rstat(struct styx2000_req *req, uint8* buf) {
  struct styx2000_fcall *f = &req->ofcall;
  PBIT16(buf, f->parlen);
  buf += BIT16SZ;
  PBIT16(buf, f->nstat);
  buf += BIT16SZ;
  PBIT16(buf, f->stat.type);
  buf += BIT16SZ;
  PBIT32(buf, f->stat.dev);
  buf += BIT32SZ;
  
  PBIT8(buf, f->stat.qid.type);
  buf += BIT8SZ;
  PBIT32(buf, f->stat.qid.vers);
  buf += BIT32SZ;
  PBIT64(buf, f->stat.qid.path);
  buf += BIT64SZ;

  PBIT32(buf, f->stat.mode);
  buf += BIT32SZ;
  PBIT32(buf, f->stat.atime);
  buf += BIT32SZ;
  PBIT32(buf, f->stat.mtime);
  buf += BIT32SZ;
  PBIT64(buf, f->stat.length);
  buf += BIT64SZ;
  buf = styx2000_pstring(buf, f->stat.name);
  buf = styx2000_pstring(buf, f->stat.uid);
  buf = styx2000_pstring(buf, f->stat.gid);
  buf = styx2000_pstring(buf, f->stat.muid);
  return 0;
}

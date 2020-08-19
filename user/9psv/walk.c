#include "user.h"
#include "param.h"
#include "styx2000.h"
#include "net/byteorder.h"
#include "fcall.h"

uint8_t* styx2000_parse_twalk(struct styx2000_fcall *fcall, uint8_t* buf, int len) {
  uint8_t *ep = buf + len;
  fcall->fid = GBIT32(buf);
  buf += 4;
  fcall->newfid = GBIT32(buf);
  buf += 4;
  fcall->nwname = GBIT16(buf);
  buf += 2;
  for (int i = 0; i < fcall->nwname; i++) {
    buf = styx2000_gstring(buf, ep, &fcall->wname[i]);
    if (buf == 0) {
      return 0;
    }
  }
  return buf;
}

int styx2000_compose_rwalk(struct styx2000_fcall *f, uint8_t* buf) {
  PBIT16(buf, f->nwqid);
  buf += BIT16SZ;
  for (int i = 0; i < f->nwqid; i++) {
    PBIT8(buf, f->wqid[i]->type);
    buf += BIT8SZ;
    PBIT32(buf, f->wqid[i]->vers);
    buf += BIT32SZ;
    PBIT64(buf, f->wqid[i]->path);
    buf += BIT64SZ;
  }
  return 0;
}

#include "user.h"
#include "../p9.h"
#include "net/byteorder.h"

uint8_t* parse_tcreate(struct p9_fcall *f, uint8_t* buf, int len) {
  uint8_t *ep = buf + len;
  f->fid = GBIT32(buf);
  buf += BIT32SZ;
  buf = p9_gstring(buf, ep, &f->name);
  f->perm = GBIT32(buf);
  buf += BIT32SZ;
  f->mode = GBIT8(buf);
  buf += BIT8SZ;
  return buf;
}

int compose_rcreate(struct p9_fcall *f, uint8_t* buf) {
  PBIT8(buf, f->qid.type);
  buf += BIT8SZ;
  PBIT32(buf, f->qid.vers);
  buf += BIT32SZ;
  PBIT64(buf, f->qid.path);
  buf += BIT64SZ;
  PBIT32(buf, f->iounit);
  buf += BIT32SZ;
  return 0;
}

int size_tcreate(struct p9_fcall *f) {
  int n = 0;
  n += BIT32SZ;
  n += p9_stringsz(f->name);
  n += BIT32SZ;
  n += BIT8SZ;
  return n;
}

int size_rcreate(struct p9_fcall *f) {
  int n = 0;
  n += P9_QIDSZ;
  n += BIT32SZ;
  return n;
}

void debug_tcreate(struct p9_fcall* f) {
  printf("<= TCREATE: fid: %d, name: %s, perm: %d, mode: %d\n",
    f->fid, f->name, f->perm, f->mode);
}

void debug_rcreate(struct p9_fcall* f) {
  printf("=> RCREATE: qid: { type: %d, vers: %d, path: %d }, iounit: %d\n", 
    f->qid.type, f->qid.vers, f->qid.path, f->iounit);
}

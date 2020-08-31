#include "user.h"
#include "../p9.h"
#include "net/byteorder.h"

uint8_t* parse_tattach(struct p9_fcall *f, uint8_t* buf, int len) {
  uint8_t *ep = buf + len;
  f->fid = GBIT32(buf);
  buf += 4;
  f->afid = GBIT32(buf);
  buf += 4;
  buf = p9_gstring(buf, ep, &f->uname);
  if (buf == 0) {
    return 0;
  }
  buf = p9_gstring(buf, ep, &f->aname);
  if (buf == 0) {
    return 0;
  }
  return buf;
}

int compose_rattach(struct p9_fcall *f, uint8_t* buf) {
  PBIT8(buf, f->qid.type);
  buf += BIT8SZ;
  PBIT32(buf, f->qid.vers);
  buf += BIT32SZ;
  PBIT64(buf, f->qid.path);
  buf += BIT64SZ;
  return 0;
}

int size_tattach(struct p9_fcall *f) {
  int n = 0;
  n += BIT32SZ;
  n += BIT32SZ;
  n += p9_stringsz(f->uname);
  n += p9_stringsz(f->aname);
  return n;
}

int size_rattach(struct p9_fcall *f) {
  int n = 0;
  n += P9_QIDSZ;
  return n;
}

void debug_tattach(struct p9_fcall* f) {
  printf("<= TATTACH: fid: %d, afid: %d, uname: %s, aname: %s\n",
    f->fid, f->afid, f->uname, f->aname);
}

void debug_rattach(struct p9_fcall* f) {
  printf("=> RATTACH: qid { type: %d, vers: %d, path: %d }\n",
    f->qid.type, f->qid.vers, f->qid.path);
}

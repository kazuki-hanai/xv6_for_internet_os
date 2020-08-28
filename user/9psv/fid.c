#include "user.h"
#include "p9.h"
#include "net/byteorder.h"

static void freefid(struct p9_fid*);

static void incfidref(void *v) {
  struct p9_fid *f;
  f = v;
  if (f) {
    // incref
  }
}

struct p9_fidpool* p9_allocfidpool() {
  struct p9_fidpool *fpool;
  fpool = p9malloc(sizeof *fpool);
  fpool->destroy = freefid;
  if ((fpool->map = allocmap(incfidref)) == 0) {
    free(fpool);
    return 0;
  }
  return fpool;
}

void p9_freefidpool(struct p9_fidpool *fpool) {
  freemap(fpool->map, (void (*)(void *))fpool->destroy);
  free(fpool);
}

struct p9_fid* p9_lookupfid(struct p9_fidpool *fpool, uint64_t fid) {
  return lookupkey(fpool->map, fid);
}

struct p9_fid* p9_removefid(struct p9_fidpool *fpool, uint64_t fid) {
  return deletekey(fpool->map, fid);
}

struct p9_fid* p9_allocfid(
  struct p9_fidpool* fpool,
  uint64_t fid,
  struct p9_qid* qid
) {
  struct p9_fid *f;
  f = p9malloc(sizeof *f);
  f->fid = fid;
  f->qid = qid;
  if (qid != 0) {
    qid->inc(qid);
  }
  f->fpool = fpool;
  f->offset = 0;
  f->buf = p9malloc(P9_MAXMSGLEN);
  if (caninsertkey(fpool->map, fid, f) == 0) {
    freefid(f);
    return 0;
  }

  return f;
}

static void freefid(struct p9_fid* fid) {
  // TODO free qid
  struct p9_qid *qid = fid->qid;
  if (qid != 0) {
    qid->dec(qid);
    if (!qid->is_referenced(qid)) {
      qid = p9_removeqid(qid->qpool, qid->path);
      qid->qpool->destroy(qid);
    }
  }
  p9_removefid(fid->fpool, fid->fid);
  free(fid->buf);
  free(fid);
}

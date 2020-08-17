#include "user.h"
#include "types.h"
#include "param.h"
#include "styx2000.h"
#include "net/byteorder.h"
#include "fcall.h"

static void freefid(struct styx2000_fid*);

static void incfidref(void *v) {
  struct styx2000_fid *f;
  f = v;
  if (f) {
    // incref
  }
}

struct styx2000_fidpool* styx2000_allocfidpool() {
  struct styx2000_fidpool *fpool;
  fpool = malloc(sizeof *fpool);
  if (fpool == 0) {
    return 0;
  }
  fpool->destroy = freefid;
  if ((fpool->map = allocmap(incfidref)) == 0) {
    free(fpool);
    return 0;
  }
  return fpool;
}

void styx2000_freefidpool(struct styx2000_fidpool *fpool) {
  freemap(fpool->map, (void (*)(void *))fpool->destroy);
  free(fpool);
}

struct styx2000_fid* styx2000_lookupfid(struct styx2000_fidpool *fpool, uint64 fid) {
  return lookupkey(fpool->map, fid);
}

struct styx2000_fid* styx2000_removefid(struct styx2000_fidpool *fpool, uint64 fid) {
  return deletekey(fpool->map, fid);
}

struct styx2000_fid* styx2000_allocfid(
  struct styx2000_fidpool* fpool,
  uint64 fid,
  struct styx2000_qid* qid
) {
  struct styx2000_fid *f;
  f = malloc(sizeof *f);
  if (f == 0) {
    return 0;
  }
  f->fid = fid;
  f->qid = qid;
  if (qid != 0) {
    qid->inc(qid);
  }
  f->fpool = fpool;
  if (caninsertkey(fpool->map, fid, f) == 0) {
    freefid(f);
    return 0;
  }

  return f;
}

static void freefid(struct styx2000_fid* fid) {
  // TODO free qid
  struct styx2000_qid *qid = fid->qid;
  if (qid != 0) {
    qid->dec(qid);
    if (!qid->is_referenced(qid)) {
      qid = styx2000_removeqid(qid->qpool, qid->path);
      qid->qpool->destroy(qid);
    }
  }
  free(fid);
}

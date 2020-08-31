#include "user.h"
#include "p9.h"

int  p9_compose_rversion(struct p9_fcall*, uint8_t*);
int  p9_compose_rattach(struct p9_fcall*, uint8_t*);
int  p9_compose_rerror(struct p9_fcall*, uint8_t*);
int  p9_compose_rflush(struct p9_fcall*, uint8_t*);
int  p9_compose_rwalk(struct p9_fcall*, uint8_t*);
int  p9_compose_ropen(struct p9_fcall*, uint8_t*);
int  p9_compose_rcreate(struct p9_fcall*, uint8_t*);
int  p9_compose_rread(struct p9_fcall*, uint8_t*);
int  p9_compose_rwrite(struct p9_fcall*, uint8_t*);
int  p9_compose_rclunk(struct p9_fcall*, uint8_t*);
int  p9_compose_rremove(struct p9_fcall*, uint8_t*);
int  p9_compose_rstat(struct p9_fcall*, uint8_t*);
int  p9_compose_rwstat(struct p9_fcall*, uint8_t*);

static int compose_ignore(struct p9_fcall* f, uint8_t* buf) {
  return 0;
}

static int (*composefunc[]) (struct p9_fcall* f, uint8_t* buf) = {
  [P9_RVERSION]     p9_compose_rversion,
  [P9_RAUTH]        compose_ignore,
  [P9_RATTACH]      p9_compose_rattach,
  [P9_RERROR]       p9_compose_rerror,
  [P9_RFLUSH]       p9_compose_rflush,
  [P9_RWALK]        p9_compose_rwalk,
  [P9_ROPEN]        p9_compose_ropen,
  [P9_RCREATE]      p9_compose_rcreate,
  [P9_RREAD]        p9_compose_rread,
  [P9_RWRITE]       p9_compose_rwrite,
  [P9_RCLUNK]       p9_compose_rclunk,
  [P9_RREMOVE]      p9_compose_rremove,
  [P9_RSTAT]        p9_compose_rstat,
  [P9_RWSTAT]       p9_compose_rwstat,
};

int p9_composefcall(struct p9_fcall *f, uint8_t* buf, int size) {
  PBIT32(buf, f->size);
  buf += BIT32SZ;
  PBIT8(buf, f->type);
  buf += BIT8SZ;
  PBIT16(buf, f->tag);
  buf += BIT16SZ;

  if (composefunc[f->type](f, buf) == -1) {
    return -1;
  }

  return 0;
}

#include "user.h"
#include "../p9.h"

int size_tversion(struct p9_fcall *f);
int size_rversion(struct p9_fcall *f);
int size_tauth(struct p9_fcall *f);
int size_rauth(struct p9_fcall *f);
int size_tattach(struct p9_fcall *f);
int size_rattach(struct p9_fcall *f);
int size_rerror(struct p9_fcall *f);
int size_tflush(struct p9_fcall *f);
int size_rflush(struct p9_fcall *f);
int size_twalk(struct p9_fcall *f);
int size_rwalk(struct p9_fcall *f);
int size_topen(struct p9_fcall *f);
int size_ropen(struct p9_fcall *f);
int size_tcreate(struct p9_fcall *f);
int size_rcreate(struct p9_fcall *f);
int size_tread(struct p9_fcall *f);
int size_rread(struct p9_fcall *f);
int size_twrite(struct p9_fcall *f);
int size_rwrite(struct p9_fcall *f);
int size_tclunk(struct p9_fcall *f);
int size_rclunk(struct p9_fcall *f);
int size_tremove(struct p9_fcall *f);
int size_rremove(struct p9_fcall *f);
int size_tstat(struct p9_fcall *f);
int size_rstat(struct p9_fcall *f);
int size_twstat(struct p9_fcall *f);
int size_rwstat(struct p9_fcall *f);

static int (*sizefunc[]) (struct p9_fcall* f) = {
  [P9_TVERSION]     size_tversion,
  [P9_RVERSION]     size_rversion,
  [P9_TAUTH]        size_tauth,
  [P9_RAUTH]        size_rauth,
  [P9_TATTACH]      size_tattach,
  [P9_RATTACH]      size_rattach,
  [P9_RERROR]       size_rerror,
  [P9_TFLUSH]       size_tflush,
  [P9_RFLUSH]       size_rflush,
  [P9_TWALK]        size_twalk,
  [P9_RWALK]        size_rwalk,
  [P9_TOPEN]        size_topen,
  [P9_ROPEN]        size_ropen,
  [P9_TCREATE]      size_tcreate,
  [P9_RCREATE]      size_rcreate,
  [P9_TREAD]        size_tread,
  [P9_RREAD]        size_rread,
  [P9_TWRITE]       size_twrite,
  [P9_RWRITE]       size_rwrite,
  [P9_TCLUNK]       size_tclunk,
  [P9_RCLUNK]       size_rclunk,
  [P9_TREMOVE]      size_tremove,
  [P9_RREMOVE]      size_rremove,
  [P9_TSTAT]        size_tstat,
  [P9_RSTAT]        size_rstat,
  [P9_TWSTAT]       size_twstat,
  [P9_RWSTAT]       size_rwstat,
};

uint32_t p9_getfcallsize(struct p9_fcall *f) {
  uint32_t n;

	n = 0;
	n += BIT32SZ;	/* size */
	n += BIT8SZ;	/* type */
	n += BIT16SZ;	/* tag */

  n += sizefunc[f->type](f);
  return n;
}

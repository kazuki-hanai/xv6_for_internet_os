#include "user.h"
#include "../p9.h"

void debug_tversion(struct p9_fcall *f);
void debug_rversion(struct p9_fcall *f);
void debug_tauth(struct p9_fcall *f);
void debug_rauth(struct p9_fcall *f);
void debug_tattach(struct p9_fcall *f);
void debug_rattach(struct p9_fcall *f);
void debug_rerror(struct p9_fcall *f);
void debug_tflush(struct p9_fcall *f);
void debug_rflush(struct p9_fcall *f);
void debug_twalk(struct p9_fcall *f);
void debug_rwalk(struct p9_fcall *f);
void debug_topen(struct p9_fcall *f);
void debug_ropen(struct p9_fcall *f);
void debug_tcreate(struct p9_fcall *f);
void debug_rcreate(struct p9_fcall *f);
void debug_tread(struct p9_fcall *f);
void debug_rread(struct p9_fcall *f);
void debug_twrite(struct p9_fcall *f);
void debug_rwrite(struct p9_fcall *f);
void debug_tclunk(struct p9_fcall *f);
void debug_rclunk(struct p9_fcall *f);
void debug_tremove(struct p9_fcall *f);
void debug_rremove(struct p9_fcall *f);
void debug_tstat(struct p9_fcall *f);
void debug_rstat(struct p9_fcall *f);
void debug_twstat(struct p9_fcall *f);
void debug_rwstat(struct p9_fcall *f);

static void (*debugfunc[]) (struct p9_fcall* f) = {
	[P9_TVERSION]     debug_tversion,
	[P9_RVERSION]     debug_rversion,
	[P9_TAUTH]        debug_tauth,
	[P9_RAUTH]        debug_rauth,
	[P9_TATTACH]      debug_tattach,
	[P9_RATTACH]      debug_rattach,
	[P9_RERROR]       debug_rerror,
	[P9_TFLUSH]       debug_tflush,
	[P9_RFLUSH]       debug_rflush,
	[P9_TWALK]        debug_twalk,
	[P9_RWALK]        debug_rwalk,
	[P9_TOPEN]        debug_topen,
	[P9_ROPEN]        debug_ropen,
	[P9_TCREATE]      debug_tcreate,
	[P9_RCREATE]      debug_rcreate,
	[P9_TREAD]        debug_tread,
	[P9_RREAD]        debug_rread,
	[P9_TWRITE]       debug_twrite,
	[P9_RWRITE]       debug_rwrite,
	[P9_TCLUNK]       debug_tclunk,
	[P9_RCLUNK]       debug_rclunk,
	[P9_TREMOVE]      debug_tremove,
	[P9_RREMOVE]      debug_rremove,
	[P9_TSTAT]        debug_tstat,
	[P9_RSTAT]        debug_rstat,
	[P9_TWSTAT]       debug_twstat,
	[P9_RWSTAT]       debug_rwstat,
};

void p9_debugfcall(struct p9_fcall *f) {
	debugfunc[f->type](f);
}

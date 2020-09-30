#include "user.h"
#include "../p9.h"

int  compose_rversion(struct p9_fcall*, uint8_t*);
int  compose_rattach(struct p9_fcall*, uint8_t*);
int  compose_rerror(struct p9_fcall*, uint8_t*);
int  compose_rflush(struct p9_fcall*, uint8_t*);
int  compose_rwalk(struct p9_fcall*, uint8_t*);
int  compose_ropen(struct p9_fcall*, uint8_t*);
int  compose_rcreate(struct p9_fcall*, uint8_t*);
int  compose_rread(struct p9_fcall*, uint8_t*);
int  compose_rwrite(struct p9_fcall*, uint8_t*);
int  compose_rclunk(struct p9_fcall*, uint8_t*);
int  compose_rremove(struct p9_fcall*, uint8_t*);
int  compose_rstat(struct p9_fcall*, uint8_t*);
int  compose_rwstat(struct p9_fcall*, uint8_t*);

static int compose_ignore(struct p9_fcall* f, uint8_t* buf) {
	return 0;
}

static int (*composefunc[]) (struct p9_fcall* f, uint8_t* buf) = {
	[P9_RVERSION]     compose_rversion,
	[P9_RAUTH]        compose_ignore,
	[P9_RATTACH]      compose_rattach,
	[P9_RERROR]       compose_rerror,
	[P9_RFLUSH]       compose_rflush,
	[P9_RWALK]        compose_rwalk,
	[P9_ROPEN]        compose_ropen,
	[P9_RCREATE]      compose_rcreate,
	[P9_RREAD]        compose_rread,
	[P9_RWRITE]       compose_rwrite,
	[P9_RCLUNK]       compose_rclunk,
	[P9_RREMOVE]      compose_rremove,
	[P9_RSTAT]        compose_rstat,
	[P9_RWSTAT]       compose_rwstat,
};

int composefcall(struct p9_fcall *f, uint8_t* buf, int size) {
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

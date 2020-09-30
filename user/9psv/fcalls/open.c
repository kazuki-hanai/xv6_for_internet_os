#include "user.h"
#include "../p9.h"
#include "net/byteorder.h"

uint8_t* parse_topen(struct p9_fcall *f, uint8_t* buf, int len) {
	f->fid = GBIT32(buf);
	buf += BIT32SZ;
	f->mode = GBIT8(buf);
	buf += BIT8SZ;
	return buf;
}

int compose_ropen(struct p9_fcall *f, uint8_t* buf) {
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

int size_topen(struct p9_fcall *f) {
	int n = 0;
	n += BIT32SZ;
	n += BIT8SZ;
	return n;
}

int size_ropen(struct p9_fcall *f) {
	int n = 0;
	n += P9_QIDSZ;
	n += BIT32SZ;
	return n;
}

void debug_topen(struct p9_fcall* f) {
	printf("<= TOPEN: fid: %d, mode: %d\n", f->fid, f->mode);
}

void debug_ropen(struct p9_fcall* f) {
	printf("=> ROPEN: qid: { type: %d, vers: %d, path: %d }, iounit: %d\n", 
		f->qid.type, f->qid.vers, f->qid.path, f->iounit);
}

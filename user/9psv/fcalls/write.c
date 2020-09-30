#include "user.h"
#include "../p9.h"
#include "net/byteorder.h"

uint8_t* parse_twrite(struct p9_fcall *f, uint8_t* buf, int len) {
	f->fid = GBIT32(buf);
	buf += BIT32SZ;
	f->offset = GBIT64(buf);
	buf += BIT64SZ;
	f->count = GBIT32(buf);
	buf += BIT32SZ;
	f->count = f->count >= P9_MAXDATALEN ? P9_MAXDATALEN : f->count;
	f->data = p9malloc(f->count);
	for (int i = 0; i < f->count; i++) {
		f->data[i] = GBIT8(buf);
		buf += BIT8SZ;
	}
	return buf;
}

int compose_rwrite(struct p9_fcall *f, uint8_t* buf) {
	PBIT32(buf, f->count);
	buf += BIT32SZ;
	return 0;
}

int size_twrite(struct p9_fcall *f) {
	int n = 0;
	n += BIT32SZ;
	n += BIT64SZ;
	n += BIT32SZ;
	n += f->count;
	return n;
}

int size_rwrite(struct p9_fcall *f) {
	int n = 0;
	n += BIT32SZ;
	return n;
}

void debug_twrite(struct p9_fcall* f) {
	printf("<= TWRITE: fid: %d, offset: %d, count: %d, data: [...]\n",
		f->fid, f->offset, f->count);
}

void debug_rwrite(struct p9_fcall* f) {
	printf("=> RWRITE: count: %d\n", f->count);
}

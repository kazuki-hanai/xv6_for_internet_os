#include "user.h"
#include "../p9.h"
#include "net/byteorder.h"

uint8_t* parse_tread(struct p9_fcall *f, uint8_t* buf, int len) {
	f->fid = GBIT32(buf);
	buf += BIT32SZ;
	f->offset = GBIT64(buf);
	buf += BIT64SZ;
	f->count = GBIT32(buf);
	buf += BIT32SZ;
	return buf;
}

int compose_rread(struct p9_fcall *f, uint8_t* buf) {
	PBIT32(buf, f->count);
	buf += BIT32SZ;
	for (int i = 0; i < f->count; i++) {
		PBIT8(buf, f->data[i]);
		buf += 1;
	}
	return 0;
}

int size_tread(struct p9_fcall *f) {
	int n = 0;
	n += BIT32SZ;
	n += BIT64SZ;
	n += BIT32SZ;
	return n;
}

int size_rread(struct p9_fcall *f) {
	int n = 0;
	n += BIT32SZ;
	n += f->count;
	return n;
}

void debug_tread(struct p9_fcall* f) {
	printf("<= TREAD: fid: %d, offset: %d, count: %d\n",
		f->fid, f->offset, f->count);
}

void debug_rread(struct p9_fcall* f) {
	printf("=> RREAD: count: %d, data: [...]\n", f->count);
}

#include "user.h"
#include "../p9.h"
#include "net/byteorder.h"

uint8_t* parse_tversion(struct p9_fcall *f, uint8_t* buf, int len) {
	if (f->tag != P9_NOTAG) {
		return 0;
	}
	uint8_t *ep = buf + len;
	f->msize = GBIT32(buf);
	buf += BIT32SZ;
	buf = p9_gstring(buf, ep, &f->version);
	if (buf == 0) {
		return 0;
	}
	return buf;
}

int compose_rversion(struct p9_fcall *f, uint8_t* buf) {
	PBIT32(buf, f->msize);
	buf += BIT32SZ;
	buf = p9_pstring(buf, f->version);
	return 0;
}

int size_tversion(struct p9_fcall *f) {
	int n = 0;
	n += BIT32SZ;
	n += p9_stringsz(f->version);
	return n;
}

int size_rversion(struct p9_fcall *f) {
	int n = 0;
	n += BIT32SZ;
	n += p9_stringsz(f->version);
	return n;
}

void debug_tversion(struct p9_fcall* f) {
	printf("<= TVERSION: %s\n", f->version);
}

void debug_rversion(struct p9_fcall* f) {
	printf("=> RVERSION: %s\n", f->version);
}

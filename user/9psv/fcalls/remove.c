#include "user.h"
#include "../p9.h"
#include "net/byteorder.h"

uint8_t* parse_tremove(struct p9_fcall *f, uint8_t* buf, int len) {
	f->fid = GBIT32(buf);
	buf += BIT32SZ;
	return buf;
}

int compose_rremove(struct p9_fcall *f, uint8_t* buf) {
	return 0;
}

int size_tremove(struct p9_fcall *f) {
	int n = 0;
	n += BIT32SZ;
	return n;
}

int size_rremove(struct p9_fcall *f) {
	int n = 0;
	return n;
}

void debug_tremove(struct p9_fcall* f) {
	printf("<= TREMOVE: fid: %d\n", f->fid);
}

void debug_rremove(struct p9_fcall* f) {
	printf("=> RREMOVE: \n");
}

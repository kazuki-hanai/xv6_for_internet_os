#include "user.h"
#include "../p9.h"
#include "net/byteorder.h"

uint8_t* parse_tauth(struct p9_fcall *f, uint8_t* buf, int len) {
	return 0;
}

int compose_rauth(struct p9_fcall *f, uint8_t* buf) {
	return 0;
}

int size_tauth(struct p9_fcall *f) {
	int n = 0;
	n += BIT32SZ;
	n += p9_stringsz(f->uname);
	n += p9_stringsz(f->aname);
	return n;
}

int size_rauth(struct p9_fcall *f) {
	int n = 0;
	n += P9_QIDSZ;
	return n;
}

void debug_tauth(struct p9_fcall* f) {
	printf("<= TAUTH: \n");
}

void debug_rauth(struct p9_fcall* f) {
	printf("=> RAUTH: \n");
}

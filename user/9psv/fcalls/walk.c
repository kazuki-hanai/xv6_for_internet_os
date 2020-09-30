#include "user.h"
#include "../p9.h"
#include "net/byteorder.h"

uint8_t* parse_twalk(struct p9_fcall *f, uint8_t* buf, int len) {
	uint8_t *ep = buf + len;
	f->fid = GBIT32(buf);
	buf += BIT32SZ;
	f->newfid = GBIT32(buf);
	buf += BIT32SZ;
	f->nwname = GBIT16(buf);
	buf += BIT16SZ;
	for (int i = 0; i < f->nwname; i++) {
		buf = p9_gstring(buf, ep, &f->wname[i]);
		if (buf == 0) {
			return 0;
		}
	}
	return buf;
}

int compose_rwalk(struct p9_fcall *f, uint8_t* buf) {
	PBIT16(buf, f->nwqid);
	buf += BIT16SZ;
	for (int i = 0; i < f->nwqid; i++) {
		PBIT8(buf, f->wqid[i].type);
		buf += BIT8SZ;
		PBIT32(buf, f->wqid[i].vers);
		buf += BIT32SZ;
		PBIT64(buf, f->wqid[i].path);
		buf += BIT64SZ;
	}
	return 0;
}

int size_twalk(struct p9_fcall *f) {
	int n = 0;
	n += BIT32SZ;
	n += BIT32SZ;
	n += BIT16SZ;
	for(int i = 0; i < f->nwname; i++)
		n += p9_stringsz(f->wname[i]);
	return n;
}

int size_rwalk(struct p9_fcall *f) {
	int n = 0;
	n += BIT16SZ;
	n += f->nwqid*P9_QIDSZ;
	return n;
}

void debug_twalk(struct p9_fcall* f) {
	printf("<= TWALK: fid: %d, newfid: %d, nwname: %d\n",
		f->fid, f->newfid, f->nwname);
	for (int i = 0; i < f->nwname; i++) {
		printf("\twname: %s\n", f->wname[i]);
	}
}

void debug_rwalk(struct p9_fcall* f) {
	printf("=> RWALK: nwqid: %d\n", f->nwqid);
	for (int i = 0; i < f->nwqid; i++) {
		printf("wqid[%d] { type: %d, vers: %d, path: %d }\n",
			i, f->wqid[i].type, f->wqid[i].vers, f->wqid[i].path);
	}
}

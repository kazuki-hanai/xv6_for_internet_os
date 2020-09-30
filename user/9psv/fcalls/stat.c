#include "user.h"
#include "stat.h"
#include "fcntl.h"
#include "../p9.h"
#include "net/byteorder.h"

uint8_t* parse_tstat(struct p9_fcall *f, uint8_t* buf, int len) {
	f->fid = GBIT32(buf);
	buf += BIT32SZ;
	return buf;
}

uint8_t* parse_twstat(struct p9_fcall *f, uint8_t* buf, int len) {
	// TODO
	return buf;
}

int compose_rstat(struct p9_fcall *f, uint8_t* buf) {
	PBIT16(buf, f->parlen);
	buf += BIT16SZ;
	PBIT16(buf, f->nstat);
	buf += BIT16SZ;
	PBIT16(buf, f->stat->type);
	buf += BIT16SZ;
	PBIT32(buf, f->stat->dev);
	buf += BIT32SZ;
	
	PBIT8(buf, f->stat->qid.type);
	buf += BIT8SZ;
	PBIT32(buf, f->stat->qid.vers);
	buf += BIT32SZ;
	PBIT64(buf, f->stat->qid.path);
	buf += BIT64SZ;

	PBIT32(buf, f->stat->mode);
	buf += BIT32SZ;
	PBIT32(buf, f->stat->atime);
	buf += BIT32SZ;
	PBIT32(buf, f->stat->mtime);
	buf += BIT32SZ;
	PBIT64(buf, f->stat->length);
	buf += BIT64SZ;
	buf = p9_pstring(buf, f->stat->name);
	buf = p9_pstring(buf, f->stat->uid);
	buf = p9_pstring(buf, f->stat->gid);
	buf = p9_pstring(buf, f->stat->muid);
	return 0;
}

int compose_rwstat(struct p9_fcall *f, uint8_t* buf) {
	return 0;
}

int size_tstat(struct p9_fcall *f) {
	int n = 0;
	n += BIT32SZ;
	return n;
}

int size_rstat(struct p9_fcall *f) {
	int n = 0;
	n += BIT16SZ;
	n += f->parlen;
	return n;
}

int size_twstat(struct p9_fcall *f) {
	int n = 0;
	n += BIT32SZ;
	n += BIT16SZ;
	n += f->nstat;
	return n;
}

int size_rwstat(struct p9_fcall *f) {
	int n = 0;
	return n;
}

void debug_tstat(struct p9_fcall* f) {
	printf("<= TSTAT: fid: %d\n", f->fid);
}

void debug_rstat(struct p9_fcall* f) {
	printf("=> RSTAT: nstat: %d, stat: {\n", f->nstat);
	printf("\ttype: %d\n", f->stat->type);
	printf("\tdev: %d\n", f->stat->dev);
	printf("\tqid: { type: %d, vers: %d, path: %d }\n", 
		f->stat->qid.type, f->stat->qid.vers, f->stat->qid.path);
	printf("\tmode: %d\n", f->stat->mode);
	printf("\tatime: %d\n", f->stat->atime);
	printf("\tmtime: %d\n", f->stat->mtime);
	printf("\tlength: %d\n", f->stat->length);
	printf("\tname: %s\n", f->stat->name);
	printf("\tuid: %s\n", f->stat->uid);
	printf("\tgid: %s\n", f->stat->gid);
	printf("\tmuid: %s\n", f->stat->muid);
	printf("}\n");
}

void debug_twstat(struct p9_fcall* f) {
	printf("<= TWSTAT: \n");
}

void debug_rwstat(struct p9_fcall* f) {
	printf("=> RWSTAT: \n");
}

int compose_stat(char* data, struct p9_stat *stat) {
	int len = stat->size;
	uint8_t* p = (uint8_t*)data;
	PBIT16(p, len);
	p += BIT16SZ;
	PBIT16(p, stat->type);
	p += BIT16SZ;
	PBIT32(p, stat->dev);
	p += BIT32SZ;
	
	PBIT8(p, stat->qid.type);
	p += BIT8SZ;
	PBIT32(p, stat->qid.vers);
	p += BIT32SZ;
	PBIT64(p, stat->qid.path);
	p += BIT64SZ;

	PBIT32(p, stat->mode);
	p += BIT32SZ;
	PBIT32(p, stat->atime);
	p += BIT32SZ;
	PBIT32(p, stat->mtime);
	p += BIT32SZ;
	PBIT64(p, stat->length);
	p += BIT64SZ;
	p = p9_pstring(p, stat->name);
	p = p9_pstring(p, stat->uid);
	p = p9_pstring(p, stat->gid);
	p = p9_pstring(p, stat->muid);
	return p - (uint8_t*)data;
}

struct p9_stat* p9_getstat(char *path) {
	struct p9_stat* stat;
	struct stat st;
	int fd;

	if ((fd = p9open(path, O_RDONLY)) == -1) {
		printf("[getstat] cannot open: %s\n", path);
		return 0;
	}

	stat = p9malloc(sizeof *stat);

	if (fstat(fd, &st) < 0) {
		fprintf(2, "[getstat] cannot stat path: %s\n", path);
		return 0;
	}

	// TODO time, uid, gid
	stat->type = 0;
	stat->dev = st.dev;
	stat->qid.path = (uint64_t)st.ino;
	stat->qid.vers = 0;
	stat->qid.type = to_qid_type(st.type);
	// TODO: console fix
	stat->mode = ((st.type & T_DIR) << 31) + P9_DEFPERM;
	stat->atime = 0;
	stat->mtime = 0;
	stat->length = (st.type & T_DIR) ? 0 : st.size;
	stat->name = p9_getfilename(path);
	stat->uid = "nobody";
	stat->gid = "";
	stat->muid = "";
	stat->size = P9_RSTAT_DEFLEN - 2 + strlen(stat->name) + 
		strlen(stat->uid) + strlen(stat->gid) +
		strlen(stat->muid) + BIT16SZ * 4;

	close(fd);
	return stat;
}

void p9_freestat(struct p9_stat* stat) {
	free(stat);
}

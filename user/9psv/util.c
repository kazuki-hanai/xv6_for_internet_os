#include "user.h"
#include "stat.h"
#include "fcntl.h"
#include "net/byteorder.h"
#include "net/socket.h"
#include "p9.h"

uint8_t* p9_pstring(uint8_t *p, char *s) {
	uint32_t n;

	if(s == 0){
		PBIT16(p, 0);
		p += BIT16SZ;
		return p;
	}

	n = strlen(s);
	PBIT16(p, n);
	p += BIT16SZ;
	memmove(p, s, n);
	p += n;
	return p;
}
uint8_t* p9_gstring(uint8_t* p, uint8_t* ep, char **s) {
  int n;
  if (p + 2 > ep)
    return 0;
  n = GBIT16(p);
  p += 1;
  if (p + n + 1 > ep)
    return 0;
  memmove(p, p+1, n);
  p[n] = '\0';
  *s = (char *)p;
  p += n+1;
  return p;
}

uint16_t p9_stringsz(char *s) {
	if(s == 0)
		return BIT16SZ;
	return BIT16SZ+strlen(s);
}

uint32_t p9_getfcallsize(struct p9_fcall *f) {
  uint32_t n;
	int i;

	n = 0;
	n += BIT32SZ;	/* size */
	n += BIT8SZ;	/* type */
	n += BIT16SZ;	/* tag */

	switch(f->type)
	{
	case P9_TVERSION:
		n += BIT32SZ;
		n += p9_stringsz(f->version);
		break;
	case P9_RVERSION:
		n += BIT32SZ;
		n += p9_stringsz(f->version);
		break;
	case P9_TFLUSH:
		n += BIT16SZ;
		break;
	case P9_RFLUSH:
		break;
	case P9_TAUTH:
		n += BIT32SZ;
		n += p9_stringsz(f->uname);
		n += p9_stringsz(f->aname);
		break;
  case P9_RAUTH:
		n += P9_QIDSZ;
		break;
	case P9_TATTACH:
		n += BIT32SZ;
		n += BIT32SZ;
		n += p9_stringsz(f->uname);
		n += p9_stringsz(f->aname);
		break;
  case P9_RERROR:
    n += p9_stringsz(f->ename);
	case P9_RATTACH:
		n += P9_QIDSZ;
		break;
	case P9_TWALK:
		n += BIT32SZ;
		n += BIT32SZ;
		n += BIT16SZ;
		for(i=0; i<f->nwname; i++)
			n += p9_stringsz(f->wname[i]);
		break;
	case P9_RWALK:
		n += BIT16SZ;
		n += f->nwqid*P9_QIDSZ;
		break;
	case P9_TOPEN:
		n += BIT32SZ;
		n += BIT8SZ;
		break;
	case P9_TCREATE:
		n += BIT32SZ;
		n += p9_stringsz(f->name);
		n += BIT32SZ;
		n += BIT8SZ;
		break;
	case P9_ROPEN:
	case P9_RCREATE:
		n += P9_QIDSZ;
		n += BIT32SZ;
		break;
	case P9_TWRITE:
		n += BIT32SZ;
		n += BIT64SZ;
		n += BIT32SZ;
		n += f->count;
		break;
	case P9_RWRITE:
		n += BIT32SZ;
		break;
	case P9_TREAD:
		n += BIT32SZ;
		n += BIT64SZ;
		n += BIT32SZ;
		break;
	case P9_RREAD:
		n += BIT32SZ;
		n += f->count;
		break;
	case P9_TCLUNK:
	case P9_TREMOVE:
		n += BIT32SZ;
		break;
	case P9_RREMOVE:
	case P9_RCLUNK:
		break;
	case P9_TSTAT:
		n += BIT32SZ;
		break;
	case P9_RSTAT:
		n += BIT16SZ;
		n += f->parlen;
		break;
	case P9_TWSTAT:
		n += BIT32SZ;
		n += BIT16SZ;
		n += f->nstat;
		break;
	case P9_RWSTAT:
		break;
	}
	return n;
}

int p9_composefcall(struct p9_fcall *f, uint8_t* buf, int size) {
  PBIT32(buf, f->size);
  buf += 4;
  PBIT8(buf, f->type);
  buf += 1;
  PBIT16(buf, f->tag);
  buf += 2;

  switch (f->type) {
  case P9_RVERSION:
    if (p9_compose_rversion(f, buf) == -1) {
      return -1;
    }
    break;
  case P9_RAUTH:
    return 0;
    break;
  case P9_RATTACH:
    if (p9_compose_rattach(f, buf) == -1) {
      return -1;
    }
    break;
  case P9_RERROR:
    if (p9_compose_rerror(f, buf) == -1) {
      return -1;
    }
    break;
  case P9_RWALK:
    if (p9_compose_rwalk(f, buf) == -1) {
      return -1;
    }
    break;
  case P9_ROPEN:
    if (p9_compose_ropen(f, buf) == -1) {
      return -1;
    }
    break;
  case P9_RFLUSH:
    break;
  case P9_RCREATE:
    break;
  case P9_RREAD:
    if (p9_compose_rread(f, buf) == -1) {
      return -1;
    }
    break;
  case P9_RWRITE:
    if (p9_compose_rwrite(f, buf) == -1) {
      return -1;
    }
    break;
  case P9_RCLUNK:
    if (p9_compose_rclunk(f, buf) == -1) {
      return -1;
    }
    break;
  case P9_RREMOVE:
    if (p9_compose_rremove(f, buf) == -1) {
      return -1;
    }
    break;
  case P9_RSTAT:
    if (p9_compose_rstat(f, buf) == -1) {
      return -1;
    }
    break;
  case P9_RWSTAT:
    break;
  default:
    return -1;
  }
  return 0;
}

void p9_debugfcall(struct p9_fcall *f) {
  switch (f->type) {
  case P9_TVERSION:
    printf("<= TVERSION: %s\n", f->version);
    break;
  case P9_RVERSION:
    printf("=> RVERSION: %s\n", f->version);
    break;
  case P9_TAUTH:
    printf("<= TAUTH: \n");
    break;
  case P9_RAUTH:
    printf("=> RAUTH: \n");
    break;
  case P9_TATTACH:
    printf("<= TATTACH: fid: %d, afid: %d, uname: %s, aname: %s\n",
      f->fid, f->afid, f->uname, f->aname);
    break;
  case P9_RATTACH:
    printf("=> RATTACH: qid { type: %d, vers: %d, path: %d }\n",
      f->qid->type, f->qid->vers, f->qid->path);
    break;
  case P9_RERROR:
    printf("=> RATTACH: ename: %s\n", f->ename);
  case P9_TWALK:
    printf("<= TWALK: fid: %d, newfid: %d, nwname: %d\n",
      f->fid, f->newfid, f->nwname);
    for (int i = 0; i < f->nwname; i++) {
      printf("\twname: %s\n", f->wname[i]);
    }
    break;
  case P9_RWALK:
    printf("=> RWALK: nwqid: %d\n", f->nwqid);
    for (int i = 0; i < f->nwqid; i++) {
      printf("wqid[%d] { type: %d, vers: %d, path: %d }\n",
        i, f->wqid[i]->type, f->wqid[i]->vers, f->wqid[i]->path);
    }
    break;
  case P9_TFLUSH:
    printf("<= TFLUSH: \n");
    break;
  case P9_RFLUSH:
    printf("=> RFLUSH: \n");
    break;
  case P9_TOPEN:
    printf("<= TOPEN: fid: %d, mode: %d\n", f->fid, f->mode);
    break;
  case P9_ROPEN:
    printf("=> ROPEN: qid: { type: %d, vers: %d, path: %d }, iounit: %d\n", 
      f->qid->type, f->qid->vers, f->qid->path, f->iounit);
    break;
  case P9_TCREATE:
    printf("<= TCREATE: fid: %d, name: %s, perm: %d, mode: %d\n",
      f->fid, f->name, f->perm, f->mode);
    break;
  case P9_RCREATE:
    printf("=> RCREATE: qid: { type: %d, vers: %d, path: %d }, iounit: %d\n", 
      f->qid->type, f->qid->vers, f->qid->path, f->iounit);
    break;
  case P9_TREAD:
    printf("<= TREAD: fid: %d, offset: %d, count: %d\n",
      f->fid, f->offset, f->count);
    break;
  case P9_RREAD:
    printf("=> RREAD: count: %d, data: [...]\n", f->count);
    break;
  case P9_TWRITE:
    printf("<= TWRITE: fid: %d, offset: %d, count: %d, data: [...]\n",
      f->fid, f->offset, f->count);
    break;
  case P9_RWRITE:
    printf("=> RWRITE: count: %d\n", f->count);
    break;
  case P9_TCLUNK:
    printf("<= TCLUNK: fid: %d\n", f->fid);
    break;
  case P9_RCLUNK:
    printf("=> RCLUNK: \n");
    break;
  case P9_TREMOVE:
    printf("<= TREMOVE: fid: %d\n", f->fid);
    break;
  case P9_RREMOVE:
    printf("=> RREMOVE: \n");
    break;
  case P9_TSTAT:
    printf("<= TSTAT: fid: %d\n", f->fid);
    break;
  case P9_RSTAT:
    printf("=> RSTAT: nstat: %d, stat: {\n", f->nstat);
    printf("\ttype: %d\n", f->stat->type);
    printf("\tdev: %d\n", f->stat->dev);
    printf("\tqid: { type: %d, vers: %d, path: %d }\n", 
      f->statqid->type, f->statqid->vers, f->statqid->path);
    printf("\tmode: %d\n", f->stat->mode);
    printf("\tatime: %d\n", f->stat->atime);
    printf("\tmtime: %d\n", f->stat->mtime);
    printf("\tlength: %d\n", f->stat->length);
    printf("\tname: %s\n", f->stat->name);
    printf("\tuid: %s\n", f->stat->uid);
    printf("\tgid: %s\n", f->stat->gid);
    printf("\tmuid: %s\n", f->stat->muid);
    printf("}\n");
    break;
  case P9_TWSTAT:
    printf("<= TWSTAT: \n");
    break;
  case P9_RWSTAT:
    printf("=> RWSTAT: \n");
    break;
  }
}

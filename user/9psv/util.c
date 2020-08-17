#include "user.h"
#include "types.h"
#include "stat.h"
#include "arch/riscv.h"
#include "param.h"
#include "fcntl.h"
#include "net/byteorder.h"
#include "net/socket.h"
#include "styx2000.h"
#include "fcall.h"

uint8* styx2000_gstring(uint8* p, uint8* ep, char **s) {
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

uint8* styx2000_pstring(uint8 *p, char *s) {
	uint32 n;

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

uint16 styx2000_stringsz(char *s) {
	if(s == 0)
		return BIT16SZ;
	return BIT16SZ+strlen(s);
}

uint32 styx2000_getfcallsize(struct styx2000_fcall *f) {
  uint32 n;
	int i;

	n = 0;
	n += BIT32SZ;	/* size */
	n += BIT8SZ;	/* type */
	n += BIT16SZ;	/* tag */

	switch(f->type)
	{
	case STYX2000_TVERSION:
		n += BIT32SZ;
		n += styx2000_stringsz(f->version);
		break;
	case STYX2000_RVERSION:
		n += BIT32SZ;
		n += styx2000_stringsz(f->version);
		break;
	case STYX2000_RERROR:
		n += styx2000_stringsz(f->ename);
		break;
	case STYX2000_TFLUSH:
		n += BIT16SZ;
		break;
	case STYX2000_RFLUSH:
		break;
	case STYX2000_TAUTH:
		n += BIT32SZ;
		n += styx2000_stringsz(f->uname);
		n += styx2000_stringsz(f->aname);
		break;
  case STYX2000_RAUTH:
		n += STYX2000_QIDSZ;
		break;
	case STYX2000_TATTACH:
		n += BIT32SZ;
		n += BIT32SZ;
		n += styx2000_stringsz(f->uname);
		n += styx2000_stringsz(f->aname);
		break;
	case STYX2000_RATTACH:
		n += STYX2000_QIDSZ;
		break;
	case STYX2000_TWALK:
		n += BIT32SZ;
		n += BIT32SZ;
		n += BIT16SZ;
		for(i=0; i<f->nwname; i++)
			n += styx2000_stringsz(f->wname[i]);
		break;
	case STYX2000_RWALK:
		n += BIT16SZ;
		n += f->nwqid*STYX2000_QIDSZ;
		break;
	case STYX2000_TOPEN:
	// case Topenfd:
		n += BIT32SZ;
		n += BIT8SZ;
		break;
	case STYX2000_TCREATE:
		n += BIT32SZ;
		n += styx2000_stringsz(f->name);
		n += BIT32SZ;
		n += BIT8SZ;
		break;
	case STYX2000_ROPEN:
	case STYX2000_RCREATE:
		n += STYX2000_QIDSZ;
		n += BIT32SZ;
		break;
	// case Ropenfd:
	// n += QIDSZ;
	// n += BIT32SZ;
	// n += BIT32SZ;
	// break;
	case STYX2000_TWRITE:
		n += BIT32SZ;
		n += BIT64SZ;
		n += BIT32SZ;
		n += f->count;
		break;
	case STYX2000_RWRITE:
		n += BIT32SZ;
		break;
	case STYX2000_TREAD:
		n += BIT32SZ;
		n += BIT64SZ;
		n += BIT32SZ;
		break;
	case STYX2000_RREAD:
		n += BIT32SZ;
		n += f->count;
		break;
	case STYX2000_TCLUNK:
	case STYX2000_TREMOVE:
		n += BIT32SZ;
		break;
	case STYX2000_RREMOVE:
		break;
	case STYX2000_RCLUNK:
		break;
	case STYX2000_TSTAT:
		n += BIT32SZ;
		break;
	case STYX2000_RSTAT:
		n += BIT16SZ;
		n += f->parlen;
		break;
	case STYX2000_TWSTAT:
		n += BIT32SZ;
		n += BIT16SZ;
		n += f->nstat;
		break;
	case STYX2000_RWSTAT:
		break;
	}
	return n;
}

static uint8* parse_hdr(struct styx2000_fcall *fcall, uint8* buf) {
  fcall->size = GBIT32(buf);
  buf += 4;
  fcall->type = GBIT8(buf);
  buf += 1;
  fcall->tag = GBIT16(buf);
  buf += 2;
  return buf;
}

struct styx2000_req* styx2000_parsefcall(uint8* buf, int size) {
  if (buf == 0) {
    return 0;
  }
  if (size < STYX2000_HDR_SIZE) {
    return 0;
  }

  struct styx2000_req *req = styx2000_allocreq();
  if (req == 0) {
    return 0;
  }
  memset(req, 0, sizeof *req);
  struct styx2000_fcall *ifcall = &req->ifcall;

  buf = parse_hdr(ifcall, buf);
  int mlen = ifcall->size - STYX2000_HDR_SIZE;

  switch (ifcall->type) {
    case STYX2000_TVERSION:
      buf = styx2000_parse_tversion(ifcall, buf, mlen);
      break;
    case STYX2000_TAUTH:
      break;
    case STYX2000_TATTACH:
      buf = styx2000_parse_tattach(ifcall, buf, mlen);
      break;
    case STYX2000_TWALK:
      buf = styx2000_parse_twalk(ifcall, buf, mlen);
      break;
    case STYX2000_TOPEN:
      buf = styx2000_parse_topen(ifcall, buf, mlen);
      break;
    case STYX2000_TFLUSH:
      break;
    case STYX2000_TCREATE:
      break;
    case STYX2000_TREAD:
      buf = styx2000_parse_tread(ifcall, buf, mlen);
      break;
    case STYX2000_TWRITE:
      break;
    case STYX2000_TCLUNK:
      buf = styx2000_parse_tclunk(ifcall, buf, mlen);
      break;
    case STYX2000_TREMOVE:
      break;
    case STYX2000_TSTAT:
      buf = styx2000_parse_tstat(ifcall, buf, mlen);
      break;
    case STYX2000_TWSTAT:
      break;
    default:
      goto fail;
  }

  if (buf == 0) {
    goto fail;
  }

  return req;

fail:
  if (req)
    styx2000_freereq(req);
  return 0;
}

int styx2000_composefcall(struct styx2000_req *req, uint8* buf, int size) {
  struct styx2000_fcall *f = &req->ofcall;
  PBIT32(buf, f->size);
  buf += 4;
  PBIT8(buf, f->type);
  buf += 1;
  PBIT16(buf, f->tag);
  buf += 2;

  switch (f->type) {
    case STYX2000_RVERSION:
      if (styx2000_compose_rversion(req, buf) == -1) {
        return -1;
      }
      break;
    case STYX2000_RAUTH:
      return 0;
      break;
    case STYX2000_RATTACH:
      if (styx2000_compose_rattach(req, buf) == -1) {
        return -1;
      }
      break;
    case STYX2000_RWALK:
      if (styx2000_compose_rwalk(req, buf) == -1) {
        return -1;
      }
      break;
    case STYX2000_RERROR:
      if (styx2000_compose_rerror(req, buf) == -1) {
        return -1;
      }
      break;
    case STYX2000_ROPEN:
      if (styx2000_compose_ropen(req, buf) == -1) {
        return -1;
      }
      break;
    case STYX2000_RFLUSH:
      break;
    case STYX2000_RCREATE:
      break;
    case STYX2000_RREAD:
      if (styx2000_compose_rread(req, buf) == -1) {
        return -1;
      }
      break;
    case STYX2000_RWRITE:
      break;
    case STYX2000_RCLUNK:
      if (styx2000_compose_rclunk(req, buf) == -1) {
        return -1;
      }
      break;
    case STYX2000_RREMOVE:
      break;
    case STYX2000_RSTAT:
      if (styx2000_compose_rstat(req, buf) == -1) {
        return -1;
      }
      break;
    case STYX2000_RWSTAT:
      break;
    default:
      return -1;
  }
  return 0;
}

void styx2000_debugfcall(struct styx2000_fcall *f) {
  switch (f->type) {
  case STYX2000_TVERSION:
    printf("<= TVERSION: %s\n", f->version);
    break;
  case STYX2000_RVERSION:
    printf("=> RVERSION: %s\n", f->version);
    break;
  case STYX2000_TAUTH:
    printf("<= TAUTH: \n");
    break;
  case STYX2000_RAUTH:
    printf("=> RAUTH: \n");
    break;
  case STYX2000_TATTACH:
    printf("<= TATTACH: fid: %d, afid: %d, uname: %s, aname: %s\n",
      f->fid, f->afid, f->uname, f->aname);
    break;
  case STYX2000_RATTACH:
    printf("=> RATTACH: qid { type: %d, vers: %d, path: %d }\n",
      f->qid->type, f->qid->vers, f->qid->path);
    break;
  case STYX2000_TWALK:
    printf("<= TWALK: fid: %d, newfid: %d, nwname: %d\n",
      f->fid, f->newfid, f->nwname);
    for (int i = 0; i < f->nwname; i++) {
      printf("\twname: %s\n", f->wname[i]);
    }
    break;
  case STYX2000_RWALK:
    printf("=> RWALK: nwqid: %d\n", f->nwqid);
    for (int i = 0; i < f->nwqid; i++) {
      printf("wqid[%d] { type: %d, vers: %d, path: %d }\n",
        i, f->wqid[i]->type, f->wqid[i]->vers, f->wqid[i]->path);
    }
    break;
  case STYX2000_RERROR:
    printf("=> RERROR: ename: %s\n", f->ename);
    break;
  case STYX2000_TFLUSH:
    printf("<= TFLUSH: \n");
    break;
  case STYX2000_RFLUSH:
    printf("=> RFLUSH: \n");
    break;
  case STYX2000_TOPEN:
    printf("<= TOPEN: fid: %d, mode: %d\n", f->fid, f->mode);
    break;
  case STYX2000_ROPEN:
    printf("=> ROPEN: qid: { type: %d, vers: %d, path: %d }, iounit: %d\n", 
      f->qid->type, f->qid->vers, f->qid->path, f->iounit);
    break;
  case STYX2000_TCREATE:
    printf("<= TCREATE: \n");
    break;
  case STYX2000_RCREATE:
    printf("=> RCREATE: \n");
    break;
  case STYX2000_TREAD:
    printf("<= TREAD: fid: %d, offset: %d, count: %d\n",
      f->fid, f->offset, f->count);
    break;
  case STYX2000_RREAD:
    printf("=> RREAD: count: %d, data: [...]\n", f->count);
    break;
  case STYX2000_TWRITE:
    printf("<= TWRITE: \n");
    break;
  case STYX2000_RWRITE:
    printf("=> RWRITE: \n");
    break;
  case STYX2000_TCLUNK:
    printf("<= TCLUNK: fid: %d\n", f->fid);
    break;
  case STYX2000_RCLUNK:
    printf("=> RCLUNK: \n");
    break;
  case STYX2000_TREMOVE:
    printf("<= TREMOVE: \n");
    break;
  case STYX2000_RREMOVE:
    printf("=> RREMOVE: \n");
    break;
  case STYX2000_TSTAT:
    printf("<= TSTAT: fid: %d\n", f->fid);
    break;
  case STYX2000_RSTAT:
    printf("=> RSTAT: nstat: %d, stat: {\n", f->nstat);
    printf("\ttype: %d\n", f->stat->type);
    printf("\tdev: %d\n", f->stat->dev);
    printf("\tqid: { type: %d, vers: %d, path: %d }\n", 
      f->stat->qid->type, f->stat->qid->vers, f->stat->qid->path);
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
  case STYX2000_TWSTAT:
    printf("<= TWSTAT: \n");
    break;
  case STYX2000_RWSTAT:
    printf("=> RWSTAT: \n");
    break;
  }
}

#include "user.h"
#include "styx2000.h"
#include "styx2000util.h"
#include "net/byteorder.h"
#include "fcall.h"

uint8* gstring(uint8* p, uint8* ep, char **s) {
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

uint8* pstring(uint8 *p, char *s) {
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

uint16 stringsz(char *s) {
	if(s == 0)
		return BIT16SZ;
	return BIT16SZ+strlen(s);
}

uint32 getfcallsize(struct styx2000_fcall *f) {
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
		n += stringsz(f->version);
		break;
	case STYX2000_RVERSION:
		n += BIT32SZ;
		n += stringsz(f->version);
		break;
	case STYX2000_RERROR:
		n += stringsz(f->ename);
		break;
	case STYX2000_TFLUSH:
		n += BIT16SZ;
		break;
	case STYX2000_RFLUSH:
		break;
	case STYX2000_TAUTH:
		n += BIT32SZ;
		n += stringsz(f->uname);
		n += stringsz(f->aname);
		break;
  case STYX2000_RAUTH:
		n += STYX2000_QIDSZ;
		break;
	case STYX2000_TATTACH:
		n += BIT32SZ;
		n += BIT32SZ;
		n += stringsz(f->uname);
		n += stringsz(f->aname);
		break;
	case STYX2000_RATTACH:
		n += STYX2000_QIDSZ;
		break;
	case STYX2000_TWALK:
		n += BIT32SZ;
		n += BIT32SZ;
		n += BIT16SZ;
		for(i=0; i<f->nwname; i++)
			n += stringsz(f->wname[i]);
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
		n += stringsz(f->name);
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
		// n += f->count;
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
		// n += f->nstat;
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

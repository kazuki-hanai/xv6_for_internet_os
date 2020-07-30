#pragma once

#include "types.h"
#include "file.h"
#include "net/styx2000.h"

#define VERSION9P "9P2000"

#define STYX2000_TVERSION 100
#define STYX2000_RVERSION 101
#define STYX2000_TAUTH    102
#define STYX2000_RAUTH    103
#define STYX2000_TATTACH  104
#define STYX2000_RATTACH  105
// #define STYX2000_TERROR   106
#define STYX2000_RERROR   107
#define STYX2000_TFLUSH   108
#define STYX2000_RFLUSH   109
#define STYX2000_TWALK    110
#define STYX2000_RWALK    111
#define STYX2000_TOPEN    112
#define STYX2000_ROPEN    113
#define STYX2000_TCREATE  114
#define STYX2000_RCREATE  115
#define STYX2000_TREAD    116
#define STYX2000_RREAD    117
#define STYX2000_TWRITE   118
#define STYX2000_RWRITE   119
#define STYX2000_TCLUNK   120
#define STYX2000_RCLUNK   121
#define STYX2000_TREMOVE  122
#define STYX2000_RREMOVE  123
#define STYX2000_TSTAT    124
#define STYX2000_RSTAT    125
#define STYX2000_TWSTAT   126
#define STYX2000_RWSTAT   127

#define STYX2000_NOTAG     (uint16)~0U
#define STYX2000_NOFID     (uint32)~0U
#define STYX2000_NOUID     (-1)
#define STYX2000_IOHDRSZ   24

#define STYX2000_MAXMSGLEN 2048
#define STYX2000_MAXWELEM 16

struct styx2000_qid
{
	uint64	path;
	uint64	vers;
	uint8 	type;
};

struct styx2000_fcall {
  uint32                size;
  uint8	                type;
  uint16	              tag;
  uint32	              fid;
  union {
    struct {
      uint32                msize;            /* Tversion, Rversion */
      char                  *version;         /* Tversion, Rversion */
    };
    struct {
      uint16                oldtag;           /* Tflush */
    };
    struct {
      char                  *ename;           /* Rerror */
      uint32	              errornum;	        /* Rerror 9P2000.u extension */
    };
    struct {
      struct styx2000_qid   qid;              /* Rattach, Ropen, Rcreate */
      uint32                iounit;           /* Ropen, Rcreate */
    };
    struct {
      struct styx2000_qid   aqid;             /* Rauth */
    };
    struct {
      uint32                afid;             /* Tauth, Tattach */
      char                  *uname;           /* Tauth, Tattach */
      char                  *aname;           /* Tauth, Tattach */
      uint32	              uidnum;		        /* Tauth, Tattach 9P2000.u extension */
    };
    struct {
      uint32                perm;             /* Tcreate */
      char                  *name;            /* Tcreate */
      uint8                 mode;             /* Tcreate, Topen */
      uint8	                *extension;	      /* Tcreate 9P2000.u extension */
    };
    struct {
      uint32                newfid;           /* Twalk */
      uint16                nwname;           /* Twalk */
      char                  *wname[STYX2000_MAXWELEM]; /* Twalk */
    };
    struct {
      uint16                nwqid;            /* Rwalk */
      struct styx2000_qid   wqid[STYX2000_MAXWELEM];   /* Rwalk */
    };
    struct {
      uint64                offset;           /* Tread, Twrite */
      uint32                count;            /* Tread, Twrite, Rread */
      char                  *data;            /* Twrite, Rread */
    };
    struct {
      uint16                nstat;            /* Twstat, Rstat */
      uint8                 *stat;            /* Twstat, Rstat */
    };
  };
};

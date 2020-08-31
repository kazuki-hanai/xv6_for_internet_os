#pragma once

#include "file.h"
#include "net/sock_cb.h"

#define VERSION9P "9P2000"

#define P9_PORT 564

#define P9_HDR_SIZE 7
#define P9_TRVERSION_SIZE 6

#define	P9_QIDSZ	      (BIT8SZ+BIT32SZ+BIT64SZ)
#define P9_MAXMSGLEN    8192
#define P9_MAXDATALEN   (P9_MAXMSGLEN-(P9_MAXMSGLEN+4))

#define	GBIT8(p)	((p)[0])
#define	GBIT16(p)	((p)[0]|((p)[1]<<8))
#define	GBIT32(p)	((uint32_t)((p)[0]|((p)[1]<<8)|((p)[2]<<16)|((p)[3]<<24)))
#define	GBIT64(p)	((uint32_t)((p)[0]|((p)[1]<<8)|((p)[2]<<16)|((p)[3]<<24)) |\
				((uint64_t)((p)[4]|((p)[5]<<8)|((p)[6]<<16)|((p)[7]<<24)) << 32))

#define	PBIT8(p,v)	(p)[0]=(v)
#define	PBIT16(p,v)	(p)[0]=(v);(p)[1]=(v)>>8
#define	PBIT32(p,v)	(p)[0]=(v);(p)[1]=(v)>>8;(p)[2]=(v)>>16;(p)[3]=(v)>>24
#define	PBIT64(p,v)	(p)[0]=(v);(p)[1]=(v)>>8;(p)[2]=(v)>>16;(p)[3]=(v)>>24;\
			(p)[4]=(v)>>32;(p)[5]=(v)>>40;(p)[6]=(v)>>48;(p)[7]=(v)>>56

#define	BIT8SZ		1
#define	BIT16SZ		2
#define	BIT32SZ		4
#define	BIT64SZ		8

#define P9_TVERSION 100
#define P9_RVERSION 101
#define P9_TAUTH    102
#define P9_RAUTH    103
#define P9_TATTACH  104
#define P9_RATTACH  105
#define P9_TERROR   106
#define P9_RERROR   107
#define P9_TFLUSH   108
#define P9_RFLUSH   109
#define P9_TWALK    110
#define P9_RWALK    111
#define P9_TOPEN    112
#define P9_ROPEN    113
#define P9_TCREATE  114
#define P9_RCREATE  115
#define P9_TREAD    116
#define P9_RREAD    117
#define P9_TWRITE   118
#define P9_RWRITE   119
#define P9_TCLUNK   120
#define P9_RCLUNK   121
#define P9_TREMOVE  122
#define P9_RREMOVE  123
#define P9_TSTAT    124
#define P9_RSTAT    125
#define P9_TWSTAT   126
#define P9_RWSTAT   127

#define P9_RFUNC_NUM 14

#define P9_NOTAG     (uint16_t)~0U
#define P9_NOFID     (uint32_t)~0U
#define P9_NOUID     (-1)
#define P9_IOHDRSZ   24

#define P9_MAXWELEM 16

#define P9_RSTAT_DEFLEN 41

#define	P9_OREAD	    0	/* open for read */
#define	P9_OWRITE	    1	/* write */
#define	P9_ORDWR	    2	/* read and write */
#define	P9_OEXEC	    3	/* execute, == read but check execute permission */
#define P9_OTMP       0x04
#define P9_OAUTH      0x08
#define P9_OMNTD      0x10
#define P9_OEXCL      0x20
#define P9_OAPPEND    0x40
#define P9_ODIR       0x80
#define	P9_OTRUNC	    16	/* or'ed in (except for exec), truncate file first */
#define	P9_OCEXEC	    32	/* or'ed in, close on exec */
#define	P9_ORCLOSE	  64	/* or'ed in, remove on close */
#define	P9_ODIRECT	  128	/* or'ed in, direct access */
#define	P9_ONONBLOCK  256	/* or'ed in, non-blocking call */
// #define	P9_OEXCL	    0x1000	/* or'ed in, exclusive use (create only) */
// #define	P9_OLOCK	    0x2000	/* or'ed in, lock after opening */
// #define	P9_OAPPEND	  0x4000	/* or'ed in, append only */

#define P9_MODE_DIR   0x80000000

#define P9_DEFPERM    0x1ff

#define P9_NOFILE       0
#define P9_NODIRENT     1
#define P9_FILEEXISTS   2
#define P9_NOTFOUND     3
#define P9_NOTDIR       4
#define P9_UNKNOWNFID   5
#define P9_PERM         6
#define P9_BOTCH        7
#define P9_BADOFFSET    8

#define P9_IS_DIR(t) (t & P9_ODIR)

struct intmap;

struct p9_qid {
  // qid fields
	uint64_t              path;
	uint32_t              vers;
	uint8_t 	            type;
};

struct p9_filesystem {
  struct p9_file* root;
  char*           rootpath;
  int             rootpathlen;
};

struct p9_stat {
  /* system-modified data */
  int                   size;
  uint16_t              type;   /* server type */
  uint32_t              dev;    /* server subtype */
  /* file data */
  struct p9_qid         qid;
  uint32_t              mode;   /* permissions */
  uint32_t              atime;  /* last read time */
  uint32_t              mtime;  /* last write time */
  uint64_t              length; /* file length */
  char*                 name;  /* last element of path */
  char*                 uid;   /* owner name */
  char*                 gid;   /* group name */
  char*                 muid;  /* last modifier name */
};

struct p9_file {
  int                   ref;
  struct p9_filesystem* fs;
  char*                 path;
  int                   child_num;
  char*                 childs[32];
  void*                 aux;
};

struct p9_fid {
  uint64_t            fid;
  int                 fd;
  struct p9_file*     file;
  struct p9_fidpool*  fpool;
  int                 offset;
};

struct p9_fidpool {
  struct intmap     *map;
  void              (*destroy)(struct p9_fid*);
  struct p9_server  *srv;
};

struct p9_qidpool {
  struct intmap     *map;
  void              (*destroy)(struct p9_qid*);
  struct p9_server  *srv;
};

struct p9_fcall {
  uint32_t            size;
  uint8_t             type;
  uint16_t            tag;
  uint32_t            fid;
  union {
    struct {
      uint64_t        req_mask;             /* Tgetattr */
    };
    struct {
      uint32_t        msize;                /* Tversion, Rversion */
      char            *version;             /* Tversion, Rversion */
    };
    struct {
      uint16_t        oldtag;               /* Tflush */
    };
    struct {
      const char*     ename;	              /* Rerror */
    };
    struct {
      struct p9_qid   qid;                  /* Rattach, Ropen, Rcreate */
      uint32_t        iounit;               /* Ropen, Rcreate */
    };
    struct {
      struct p9_qid   aqid;                 /* Rauth */
    };
    struct {
      uint32_t        afid;                 /* Tauth, Tattach */
      char            *uname;               /* Tauth, Tattach */
      char            *aname;               /* Tauth, Tattach */
      uint32_t        uidnum;		            /* Tauth, Tattach 9P2000.L extension */
    };
    struct {
      uint32_t        perm;                 /* Tcreate */
      char            *name;                /* Tcreate */
      uint8_t         mode;                 /* Tcreate, Topen */
      uint8_t         *extension;	          /* Tcreate 9P2000.L extension */
    };
    struct {
      uint32_t        newfid;               /* Twalk */
      uint16_t        nwname;               /* Twalk */
      char            *wname[P9_MAXWELEM];  /* Twalk */
    };
    struct {
      uint16_t        nwqid;                /* Rwalk */
      struct p9_qid   wqid[P9_MAXWELEM];    /* Rwalk */
    };
    struct {
      uint64_t        offset;               /* Tread, Twrite */
      uint32_t        count;                /* Tread, Twrite, Rread */
      char            *data;                /* Twrite, Rread */
    };
    struct {
      uint16_t        parlen;               /* Rstat */
      uint16_t        nstat;                /* Twstat, Rstat */
      struct p9_stat* stat;                 /* Twstat, Rstat */
    };
  };
};

struct p9_conn {
  int sockfd;
  uint8_t* wbuf;
  uint8_t* rbuf;
};

struct p9_req {
  struct p9_fcall ifcall;
  struct p9_fcall ofcall;
  struct p9_fid   *fid;
  int                   error;
};

// util
uint8_t                 to_qid_type(uint16_t t);
uint8_t*                p9_gstring(uint8_t*, uint8_t*, char **);
uint8_t*                p9_pstring(uint8_t *, const char *);
uint16_t                p9_stringsz(const char *);
char*                   p9_getfilename(char* path);
uint32_t                p9_getfcallsize(struct p9_fcall*);
void                    p9_debugfcall(struct p9_fcall*);
int                     p9_is_dir(uint8_t type);

struct p9_req*          parsefcall(uint8_t*, int);
int                     composefcall(struct p9_fcall*, uint8_t*, int);

// syscall wrapper
void*                   p9malloc(int);
int                     p9open(char* path, int mode);

// req
struct p9_req*          p9_allocreq();
void                    p9_freereq(struct p9_req*);
int                     p9_sendreq(struct p9_conn *conn, struct p9_req *req);
struct p9_req*          p9_recvreq(struct p9_conn *conn);

// fid
struct p9_fidpool*      p9_allocfidpool();
void                    p9_freefidpool(struct p9_fidpool*);
struct p9_fid*          p9_allocfid(struct p9_fidpool* fpool, uint64_t fid, struct p9_file* file);
struct p9_fid*          p9_lookupfid(struct p9_fidpool *, uint64_t);
struct p9_fid*          p9_removefid(struct p9_fidpool*, uint64_t);

// qid
int                     p9_getqid(char* path, struct p9_qid* qid);

// file
struct p9_file*         p9_allocfile(char* path, struct p9_filesystem* fs);
void                    p9_freefile(struct p9_file* file);
int                     p9_getdir(struct p9_file* file);

// error
const char*             p9_geterrstr(int key);
// stat
int                     compose_stat(char* data, struct p9_stat *stat);
struct p9_stat*         p9_getstat(char *path);
void                    p9_freestat(struct p9_stat* stat);

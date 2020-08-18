#pragma once

#include "types.h"
#include "file.h"
#include "styx2000.h"

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

#define STYX2000_MAXWELEM 16

#define STYX2000_RSTAT_DEFLEN 43

#define	STYX2000_OREAD	    0	/* open for read */
#define	STYX2000_OWRITE	    1	/* write */
#define	STYX2000_ORDWR	    2	/* read and write */
#define	STYX2000_OEXEC	    3	/* execute, == read but check execute permission */
#define STYX2000_OTMP       0x04
#define STYX2000_OAUTH      0x08
#define STYX2000_OMNTD      0x10
#define STYX2000_OEXCL      0x20
#define STYX2000_OAPPEND    0x40
#define STYX2000_ODIR       0x80
#define	STYX2000_OTRUNC	    16	/* or'ed in (except for exec), truncate file first */
#define	STYX2000_OCEXEC	    32	/* or'ed in, close on exec */
#define	STYX2000_ORCLOSE	  64	/* or'ed in, remove on close */
#define	STYX2000_ODIRECT	  128	/* or'ed in, direct access */
#define	STYX2000_ONONBLOCK  256	/* or'ed in, non-blocking call */
// #define	STYX2000_OEXCL	    0x1000	/* or'ed in, exclusive use (create only) */
// #define	STYX2000_OLOCK	    0x2000	/* or'ed in, lock after opening */
// #define	STYX2000_OAPPEND	  0x4000	/* or'ed in, append only */

#define STYX2000_DEFPERM    0x0777

struct intmap;
struct styx2000_qid;

struct styx2000_filesystem {
  struct styx2000_qid* root;
  char*                 rootpath;
  int                   rootpathlen;
};

struct styx2000_filelist {
  struct styx2000_qid*      qid;
  struct styx2000_filelist* filelist;
};

struct styx2000_stat {
    /* system-modified data */
    int                   size;
    uint16                type;   /* server type */
    uint32                dev;    /* server subtype */
    /* file data */
    struct styx2000_qid*  qid;    /* unique id from server */
    uint32                mode;   /* permissions */
    uint32                atime;  /* last read time */
    uint32                mtime;  /* last write time */
    uint64                length; /* file length */
    char                  *name;  /* last element of path */
    char                  *uid;   /* owner name */
    char                  *gid;   /* group name */
    char                  *muid;  /* last modifier name */
};

struct styx2000_file {
  struct styx2000_filesystem* fs;
  int                         fd;
  char*                       path;
  struct styx2000_stat*       stat;
  struct styx2000_qid*        parent;
  int                         child_num;
  struct styx2000_qid*        childs[32];
  void*                       aux;
};

struct styx2000_qid {
  // qid fields
  char*                       pathname;
	uint64	                    path;
	uint32	                    vers;
	uint8 	                    type;
  void*                       file;
  int                         ref;
  struct styx2000_qidpool*    qpool;
  void                        (*inc)(struct styx2000_qid*);
  void                        (*dec)(struct styx2000_qid*);
  int                         (*is_referenced)(struct styx2000_qid*);
};

struct styx2000_fid {
  uint64                    fid;
  struct styx2000_qid*      qid;
  struct styx2000_fidpool*  fpool;
};

struct styx2000_fidpool {
  struct intmap           *map;
  void                    (*destroy)(struct styx2000_fid*);
  struct styx2000_server  *srv;
};

struct styx2000_qidpool {
  struct intmap           *map;
  void                    (*destroy)(struct styx2000_qid*);
  struct styx2000_server  *srv;
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
      struct styx2000_qid*  qid;              /* Rattach, Ropen, Rcreate */
      uint32                iounit;           /* Ropen, Rcreate */
    };
    struct {
      struct styx2000_qid*  aqid;             /* Rauth */
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
      struct styx2000_qid*  wqid[STYX2000_MAXWELEM];   /* Rwalk */
    };
    struct {
      uint64                offset;           /* Tread, Twrite */
      uint32                count;            /* Tread, Twrite, Rread */
      char                  *data;            /* Twrite, Rread */
    };
    struct {
      uint16                parlen;           /* Rstat */
      uint16                nstat;            /* Twstat, Rstat */
      struct styx2000_stat* stat;             /* Twstat, Rstat */
    };
  };
};

// fid
struct styx2000_fidpool*  styx2000_allocfidpool();
void                      styx2000_freefidpool(struct styx2000_fidpool*);
struct styx2000_fid*      styx2000_allocfid(
  struct styx2000_fidpool* fpool,
  uint64 fid,
  struct styx2000_qid* qid
);
struct styx2000_fid*      styx2000_lookupfid(struct styx2000_fidpool *, uint64);
struct styx2000_fid*      styx2000_removefid(struct styx2000_fidpool*, uint64);

// file
uint8                     styx2000_to_qid_type(uint16);
uint8                     styx2000_to_xv6_mode(uint8);
int                       styx2000_is_dir(struct styx2000_qid* qid);
int                       styx2000_compose_stat(char*, struct styx2000_stat*);
uint64                    styx2000_getqidno(char* path);
struct styx2000_qidpool*  styx2000_allocqidpool();
void                      styx2000_freeqidpool(struct styx2000_qidpool *qpool);
struct styx2000_qid*      styx2000_lookupqid(struct styx2000_qidpool *qpool, uint64 qid);
struct styx2000_qid*      styx2000_removeqid(struct styx2000_qidpool *qpool, uint64 qid);
struct styx2000_qid*      styx2000_allocqid(
  struct styx2000_qidpool* qpool, struct styx2000_qid* parent, struct styx2000_filesystem* fs, char* path);
int                       styx2000_get_dir(struct styx2000_qid* qid, struct styx2000_filesystem* fs);

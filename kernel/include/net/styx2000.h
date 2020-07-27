#pragma once

#include "types.h"
#include "file.h"

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

#define NOTAG     (uint16)~0U
#define NOFID     (uint32)~0U
#define NOUID     (-1)
#define IOHDRSZ   24

#define MAXMSGLEN 65535

struct styx2000_header {
  uint32 size;
  uint8 type;
  uint16 tag;
};

struct styx2000_fcall {
  (char*)   (*get_message)(uint8*, int);
  (uint8*)  (*compose)(uint8*, int);
  (uint8*)  (*parse)(uint8*, int);
};

struct styx2000_fid {
  uint64 fid;
  uint32 omode;
  struct file* file;
  uint8* uid;
  struct qid qid;
  void* aux;
};

struct styx2000_qid {
  uint8 qtype;
  uint32 vers;
  uint64 uid;
}

struct styx2000_trversion {
  uint32 msize;
  char* version;
};

struct styx2000_tauth {
  uint32 afid;
  char* uname;
  char* aname;
};

struct styx2000_rauth {
  struct styx2000_qid qid;
};

struct styx2000_trerror {
  char* ename;
};

struct styx2000_tflush {
  uint16 oldtag;
};

struct styx2000_rflush {};

struct styx2000_tattach {
  uint32 fid;
  uint32 afid;
  char *uname;
  char *aname;
};

struct styx2000_rattach {
  struct styx2000_qid qid;
};

struct styx2000_twalk {
  uint32 fid;
  uint32 newfid;
  uint16 nwname;
  char* wname;
};

struct styx2000_rwalk {
  uint16 nwqid;
  struct styx2000_qid **wqid;
};

#define STYX2000_OREAD    0
#define STYX2000_OWRITE   1
#define STYX2000_ORDWR    2
#define STYX2000_OEXEC    3
#define STYX2000_NONE     4
#define STYX2000_OTRUNC   0x10
#define STYX2000_ORCLOSE  0x40
#define STYX2000_IOUNIT 8168

struct styx2000_topen {
  uint32 fid;
  uint8 mode;
};

struct styx2000_ropen {
  struct styx2000_qid qid;
  uint32 iounit;
};

struct styx2000_tcreate {
  uint32 fid;
  char* name;
  uint32 perm;
  uint8 mode;
};

struct styx2000_rcreate {
  struct styx2000_qid qid;
  uint32 iounit;
};

struct styx2000_tread {
  uint32 fid;
  uint64 offset;
  uint32 count;
};

struct styx2000_rread {
  uint32 count;
  uint8* data;  
};

struct styx2000_twrite {
  uint32 fid;
  uint64 offset;
  uint32 count;
  uint8* data;
};

struct styx2000_rwrite {
  uint32 count;
};

struct styx2000_tclunk {
  uint32 fid;
};

struct styx2000_rclunk {};

struct styx2000_tremove {
  uint32 fid;
};

struct styx2000_rremove {};

#define STYX2000_DMDIR      1 << 31
#define STYX2000_DMAPPEND   1 << 30
#define STYX2000_DMEXCL     1 << 29
#define STYX2000_DMTMP      1 << 26

struct styx2000_tstat {
  uint32 fid;
};

struct styx2000_rstat {
  // TODO design stat
  // struct stat stat;
};

struct styx2000_twstat {
  uint32 fid;
  // struct stat stat;
};

struct styx2000_rwstat {};

// parse styx2000 packet and return specific call
struct styx2000_fcall* styx2000_parsecall(uint8*, int);
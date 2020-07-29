#pragma once

#include "types.h"
#include "file.h"

#define STYX2000_PORT 5640

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

#define MAXMSGLEN 8192

struct __attribute__((__packed__)) styx2000_header {
  uint32 size;
  uint8 type;
  uint16 tag;
};

#define STYX2000_HDR_SIZE sizeof(struct styx2000_header)
#define STYX2000_TRVERSION_SIZE 6

struct styx2000_qid {
  uint8 qtype;
  uint32 vers;
  uint64 uid;
};

struct styx2000_fid {
  uint64 fid;
  uint32 omode;
  struct file* file;
  uint8* uid;
  struct styx2000_qid qid;
  void* aux;
};

struct styx2000_trversion {
  uint32 msize;
	uint16 vsize;
  uint8 *version;
};

struct styx2000_tauth {
  uint32 afid;
  uint8* uname;
  uint8* aname;
};

struct styx2000_rauth {
  struct styx2000_qid qid;
};

struct styx2000_trerror {
  uint8* ename;
};

struct styx2000_tflush {
  uint16 oldtag;
};

struct styx2000_rflush {};

struct styx2000_tattach {
  uint32 fid;
  uint32 afid;
  uint8 *uname;
  uint8 *aname;
};

struct styx2000_rattach {
  struct styx2000_qid qid;
};

struct styx2000_twalk {
  uint32 fid;
  uint32 newfid;
  uint16 nwname;
  uint8* wname;
};

struct styx2000_rwalk {
  uint16 nwqid;
  struct styx2000_qid wqid;
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
  uint8* name;
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

struct styx2000_server {};

struct styx2000_client {};


union styx2000_messages {
  struct styx2000_trversion trversion;
  struct styx2000_tauth tauth;
  struct styx2000_rauth rauth;
  struct styx2000_trerror trerror;
  struct styx2000_tflush tflush;
  struct styx2000_rflush rflush;
  struct styx2000_tattach tattach;
  struct styx2000_rattach rattach;
  struct styx2000_twalk twalk;
  struct styx2000_rwalk rwalk;
  struct styx2000_topen topen;
  struct styx2000_ropen ropen;
  struct styx2000_tcreate tcreate;
  struct styx2000_rcreate rcreate;
  struct styx2000_tread tread;
  struct styx2000_rread rread;
  struct styx2000_twrite twrite;
  struct styx2000_rwrite rwrite;
  struct styx2000_tclunk tclunk;
  struct styx2000_rclunk rclunk;
  struct styx2000_tremove tremove;
  struct styx2000_rremove rremove;
  struct styx2000_tstat tstat;
  struct styx2000_rstat rstat;
  struct styx2000_twstat twstat;
  struct styx2000_rwstat rwstat;
};

struct styx2000_fcall;
struct styx2000_message {
	struct styx2000_fcall *fcall;
  struct styx2000_header hdr;
  union styx2000_messages m;
};

struct styx2000_fcall {
  uint16 type;
  char*   (*get_message)(struct styx2000_message*);
  int  (*compose)(struct styx2000_message*, uint8**);
  int  (*parse)(struct styx2000_message*, uint8*, int);
};

// parse styx2000 packet and return specific call
struct styx2000_message* styx2000_parsecall(uint8*, int);
int styx2000_create_rversion(uint8**, uint16, uint16, uint8*);

// message
struct styx2000_message* styx2000_messagealloc();
void styx2000_messagefree(struct styx2000_message*);

// server
void styx2000_initserver();
int styx2000_serve();

#pragma once

#include "types.h"
#include "file.h"
#include "net/sock_cb.h"
#include "fcall.h"

#define STYX2000_PORT 5640

#define STYX2000_HDR_SIZE 7
#define STYX2000_TRVERSION_SIZE 6

#define	STYX2000_QIDSZ	(BIT8SZ+BIT32SZ+BIT64SZ)
#define STYX2000_MAXMSGLEN 4096

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

struct styx2000_conn {
  int sockfd;
  uint8_t* wbuf;
  uint8_t* rbuf;
};

struct styx2000_req {
  struct styx2000_fcall ifcall;
  struct styx2000_fcall ofcall;
  struct styx2000_fid   *fid;
  int                   error;
};

// util
uint8_t*                  styx2000_gstring(uint8_t*, uint8_t*, char **);
uint8_t*                  styx2000_pstring(uint8_t *, char *);
uint16_t                  styx2000_stringsz(char *);
uint32_t                  styx2000_getfcallsize(struct styx2000_fcall*);
struct styx2000_req*    styx2000_parsefcall(uint8_t*, int);
int                     styx2000_composefcall(struct styx2000_fcall*, uint8_t*, int);
void                    styx2000_debugfcall(struct styx2000_fcall*);
int                     styx2000_is_dir(uint8_t type);

// req
struct styx2000_req*    styx2000_allocreq();
void                    styx2000_freereq(struct styx2000_req*);
int                     styx2000_sendreq(struct styx2000_conn *conn, struct styx2000_req *req);
struct styx2000_req*    styx2000_recvreq(struct styx2000_conn *conn);

// version
uint8_t*                  styx2000_parse_tversion(struct styx2000_fcall*, uint8_t*, int);
int                     styx2000_compose_rversion(struct styx2000_fcall*, uint8_t*);

// attach
uint8_t*                  styx2000_parse_tattach(struct styx2000_fcall*, uint8_t*, int);
int                     styx2000_compose_rattach(struct styx2000_fcall*, uint8_t*);

// walk
uint8_t*                  styx2000_parse_twalk(struct styx2000_fcall*, uint8_t*, int);
int                     styx2000_compose_rwalk(struct styx2000_fcall*, uint8_t*);

// error
int                     styx2000_compose_rerror(struct styx2000_fcall*, uint8_t*);

// open
uint8_t*                  styx2000_parse_topen(struct styx2000_fcall*, uint8_t*, int);
int                     styx2000_compose_ropen(struct styx2000_fcall*, uint8_t*);

// stat
uint8_t*                  styx2000_parse_tstat(struct styx2000_fcall*, uint8_t*, int);
int                     styx2000_compose_rstat(struct styx2000_fcall*, uint8_t*);
int                     styx2000_compose_stat(char* data, struct styx2000_stat *stat, struct styx2000_qid *qid);
struct styx2000_stat*   styx2000_get_stat(char *path);

// read
uint8_t*                  styx2000_parse_tread(struct styx2000_fcall*, uint8_t*, int);
int                     styx2000_compose_rread(struct styx2000_fcall*, uint8_t*);

// clunk
uint8_t*                  styx2000_parse_tclunk(struct styx2000_fcall*, uint8_t*, int);
int                     styx2000_compose_rclunk(struct styx2000_fcall*, uint8_t*);

// remove
uint8_t*                  styx2000_parse_tremove(struct styx2000_fcall*, uint8_t*, int);
int                     styx2000_compose_rremove(struct styx2000_fcall*, uint8_t*);

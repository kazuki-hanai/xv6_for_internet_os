#pragma once

#include "types.h"
#include "file.h"
#include "net/sock_cb.h"
#include "fcall.h"

#define STYX2000_PORT 5640

#define STYX2000_HDR_SIZE 7
#define STYX2000_TRVERSION_SIZE 6

#define	STYX2000_QIDSZ	(BIT8SZ+BIT32SZ+BIT64SZ)

struct styx2000_req {
  struct styx2000_fcall ifcall;
  struct styx2000_fcall ofcall;
};

struct styx2000_server {
  struct sock_cb *scb;
  uint8* wbuf;
  int msize;
  int (*start)(struct styx2000_server*);
  void (*stop)(struct styx2000_server*);
  int (*send)(struct styx2000_server*, struct styx2000_req*);
};

struct styx2000_client {};


// styx2000
struct styx2000_req* styx2000_parsefcall(uint8*, int);
int styx2000_respond(struct styx2000_server*, struct styx2000_req*);

// server
void styx2000_initserver();
int styx2000_serve();

// req
struct styx2000_req* styx2000_allocreq();
void styx2000_freereq(struct styx2000_req*);

// version
uint8* styx2000_parse_rversion(struct styx2000_fcall*, uint8*, int);
int styx2000_rversion(struct styx2000_server*, struct styx2000_req*);
int styx2000_compose_rversion(struct styx2000_req*, uint8*);

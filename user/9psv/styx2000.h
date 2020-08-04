#pragma once

#include "types.h"
#include "file.h"
#include "net/sock_cb.h"
#include "fcall.h"

#define STYX2000_PORT 5640

#define STYX2000_HDR_SIZE 7
#define STYX2000_TRVERSION_SIZE 6

#define	STYX2000_QIDSZ	(BIT8SZ+BIT32SZ+BIT64SZ)
#define STYX2000_MAXMSGLEN 2048

struct styx2000_filesystem {
  char* rootpath;
};

struct styx2000_server {
  int                         sockfd;
  uint8*                      wbuf;
  uint8*                      rbuf;
  int                         msize;
  struct styx2000_filesystem  fs;
  struct styx2000_fidpool     *fpool;
  int                         (*start)(struct styx2000_server*);
  void                        (*stop)(struct styx2000_server*);
  int                         (*send)(struct styx2000_server*, struct styx2000_req*);
  struct styx2000_req*        (*recv)(struct styx2000_server*);
};

struct styx2000_client {};


// styx2000
struct styx2000_req* styx2000_parsefcall(uint8*, int);
int styx2000_respond(struct styx2000_server*, struct styx2000_req*);

// server
void styx2000_initserver();
int styx2000_serve();

// version
uint8* styx2000_parse_tversion(struct styx2000_fcall*, uint8*, int);
int styx2000_compose_rversion(struct styx2000_req*, uint8*);

// attach
uint8* styx2000_parse_tattach(struct styx2000_fcall*, uint8*, int);
int styx2000_compose_rattach(struct styx2000_req*, uint8*);

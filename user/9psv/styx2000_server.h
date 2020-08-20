#pragma once

#include "file.h"
#include "net/sock_cb.h"
#include "fcall.h"
#include "styx2000.h"

struct styx2000_server {
  struct styx2000_conn        conn;
  int                         msize;
  struct styx2000_filesystem* fs;
  struct styx2000_fidpool     *fpool;
  struct styx2000_qidpool     *qpool;
  int                         (*start)(struct styx2000_server*);
  void                        (*stop)(struct styx2000_server*);
  int                         (*send)(struct styx2000_conn*, struct styx2000_req*);
  struct styx2000_req*        (*recv)(struct styx2000_conn*);
};

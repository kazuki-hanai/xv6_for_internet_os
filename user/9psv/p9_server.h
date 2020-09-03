#pragma once

#include "file.h"
#include "net/sock_cb.h"
#include "p9.h"

struct p9_server {
  int                   done;
  int                   debug;
  struct p9_conn        conn;
  int                   msize;
  struct p9_filesystem* fs;
  struct p9_fidpool     *fpool;
  int                   (*start)(struct p9_server*);
  void                  (*stop)(struct p9_server*);
  int                   (*send)(struct p9_conn*, struct p9_req*);
  struct p9_req*        (*recv)(struct p9_conn*);
  int                   (**rfuncs) (struct p9_server* srv, struct p9_req* req);
};

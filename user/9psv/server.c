#include "user.h"
#include "fcntl.h"
#include "net/byteorder.h"
#include "net/socket.h"
#include "p9.h"
#include "p9_server.h"

static int respond(struct p9_server *srv, struct p9_req *req) {
  if (!req->error) {
    req->ofcall.type = req->ifcall.type+1;
  } else {
    req->ofcall.type = P9_RERROR;
  }

  req->ofcall.size = p9_getfcallsize(&req->ofcall);
  req->ofcall.tag = req->ifcall.tag;

  if (composefcall(&req->ofcall, srv->conn.wbuf, srv->msize) == -1 ) {
    return -1;
  }

  if (srv->send(&srv->conn, req) == -1) {
    return -1;
  }
  return 0;
}

static void err(struct p9_req* req, int errkey) {
    req->error = 1;
    req->ofcall.ename = p9_geterrstr(errkey);
}

static int rversion(struct p9_server *srv, struct p9_req *req) {
  if (strncmp(req->ifcall.version, VERSION9P, 6) != 0) {
    req->ofcall.version = "unknown";
    req->ofcall.msize = req->ifcall.msize;
    return 0;
  }
  req->ofcall.version = "9P2000";
  req->ofcall.msize = P9_MAXMSGLEN;
  return 0;
}

static int rattach(struct p9_server *srv, struct p9_req *req) {
  struct p9_file* root = srv->fs->root;
  if ((req->fid = p9_allocfid(srv->fpool, req->ifcall.fid, root)) == 0) {
    err(req, P9_NOFID);
    return 0;
  }
  // We don't support afid at present. Afid is a special fid provided to prove
  // service has a permission to attach.
  if (req->ifcall.afid != P9_NOFID) {
    // TODO: lookup afid and respond error when there is no afid
    err(req, P9_PERM);
    return 0;
  }

  if (p9_getqid(root->path, &req->ofcall.qid) < 0) {
    err(req, P9_NOFILE);
    return 0;
  }

  return 0;
}

static int rwalk(struct p9_server *srv, struct p9_req *req) {
  char path[256], *p;
  struct p9_file* par;
  struct p9_fid* fid;

  if ((fid = p9_lookupfid(srv->fpool, req->ifcall.fid)) == 0) {
    err(req, P9_UNKNOWNFID);
    return 0;
  }
  par = fid->file;

  memset(path, 0, 256);
  strcpy(path, par->path);
  p = path+strlen(par->path);

  for (uint32_t i = 0; i < req->ifcall.nwname; i++) {
    if (*(p-1) != '/') {
      *p = '/';
      p++;
    }
    strcpy(p, req->ifcall.wname[i]);
    p += strlen(req->ifcall.wname[i]);

    if (p9_getqid(path, &req->ofcall.wqid[i]) < 0) {
      err(req, P9_NOFILE);
      return 0;
    }
  }

  req->ofcall.nwqid = req->ifcall.nwname;
  if ((req->fid = p9_allocfid(srv->fpool, req->ifcall.newfid, 0)) == 0) {
    printf("[rwalk] cannot allocate newfid\n");
    return -1;
  }
  req->fid->file = p9_allocfile(path, srv->fs);

  return 0;
}

static int ropen(struct p9_server *srv, struct p9_req *req) {
  struct p9_fid *fid;
  if ((fid = p9_lookupfid(srv->fpool, req->ifcall.fid)) == 0) {
    err(req, P9_UNKNOWNFID);
    return 0;
  }
  
  if (fid->file == 0) {
    err(req, P9_NOFILE);
    return 0;
  }

  if (p9_getqid(fid->file->path, &req->ofcall.qid) < 0) {
    err(req, P9_NOFILE);
    return 0;
  }

  int mode = P9_IS_DIR(req->ofcall.qid.type) ? O_RDONLY : O_RDWR;
  if ((fid->fd = p9open(fid->file->path, mode)) == -1) {
    err(req, P9_NOFILE);
    return 0;
  }

  // TODO: mode change(req->ifcall.mode)
  return 0;
}

static int read_dir(struct p9_fid* fid, struct p9_req* req, int count) {
  struct p9_file* file = fid->file;
  if (req->ifcall.offset > 0) {
    req->ofcall.count = 0;
    return 0;
  }

  // TODO: offset
  if (p9_getdir(file) == -1) {
    return -1;
  }

  char* dp = req->ofcall.data;
  int sum = 0;
  for (int i = 0; i < file->child_num; i++) {
    char*  child   = file->childs[i];
    struct p9_stat* chstat  = p9_getstat(child);

    compose_stat(dp, chstat);
    dp += chstat->size+BIT16SZ;
    sum += chstat->size+BIT16SZ;
    free(chstat);
  }

  req->ofcall.count = sum;
  return 0;
}

static int to_offset(struct p9_fid* fid, int offset) {
  if (fid->fd == -1) {
    return -1;
  }
  if (fid->offset > offset) {
    close(fid->fd);
    if ((fid->fd = p9open(fid->file->path, O_RDWR)) < 0) {
      return -1;
    }
    fid->offset = 0;
  }
  int diff = offset - fid->offset;
  while(diff > 0) {
    int bufsize = 8192;
    char buf[bufsize];
    int size = (diff > bufsize) ? bufsize : diff;
    if ((size = read(fid->fd, buf, size)) < 0) {
      return -1;
    }
    diff -= size;
  }
  fid->offset = offset;
  return 0;
}
static int read_file(struct p9_fid* fid, struct p9_req* req, int count) {
  // TODO: offset process
  if (fid->fd == -1) {
    req->error = 1;
    req->ofcall.ename = p9_geterrstr(P9_NOFILE);
    return 0;
  }
  if (to_offset(fid, req->ifcall.offset) < 0) {
    req->error = 1;
    req->ofcall.ename = p9_geterrstr(P9_BOTCH);
    return 0;
  }
  if ((req->ofcall.count = read(fid->fd, req->ofcall.data, count)) < 0) {
    req->error = 1;
    req->ofcall.ename = p9_geterrstr(P9_NOFILE);
    return 0;
  }
  fid->offset += count;
  return 0;
}
static int rread(struct p9_server *srv, struct p9_req *req) {
  struct p9_fid *fid;
  if ((fid = p9_lookupfid(srv->fpool, req->ifcall.fid)) == 0) {
    err(req, P9_UNKNOWNFID);
    return 0;
  }

  int count = (req->ifcall.count >= P9_MAXDATALEN) ? P9_MAXDATALEN : req->ifcall.count;
  req->ofcall.data = p9malloc(count);

  struct p9_qid qid;
  if (p9_getqid(fid->file->path, &qid) < 0) {
    err(req, P9_NOFILE);
    return 0;
  }

  if (P9_IS_DIR(qid.type)) {
    read_dir(fid, req, count);
  } else {
    read_file(fid, req, count);
  }
  return 0;
}

static int rcreate(struct p9_server *srv, struct p9_req *req) {
  struct p9_fid *fid;
  if ((fid = p9_lookupfid(srv->fpool, req->ifcall.fid)) == 0) {
    err(req, P9_UNKNOWNFID);
    return 0;
  }

  char path[256], *p;
  p = path;
  strcpy(p, fid->file->path);
  p += strlen(fid->file->path);
  
  if (*(p-1) != '/') {
    *p = '/';
    p++;
  }

  strcpy(p, req->ifcall.name);

  if (req->ifcall.perm & P9_MODE_DIR) {
    if (mkdir(path) < 0){
      err(req, P9_PERM);
      return 0;
    }
  } else {
    int fd;
    if ((fd = p9open(path, O_CREATE)) < 0) {
      err(req, P9_PERM);
      return 0;
    }
    close(fd);
  }

  if (p9_getqid(path, &req->ofcall.qid) < 0) {
    err(req, P9_NOFILE);
    return 0;
  }

  srv->fpool->destroy(fid);
  if ((fid = p9_allocfid(srv->fpool, req->ifcall.fid, 0)) == 0) {
    printf("[rcreate] cannot allocate newfid\n");
    return -1;
  }
  fid->file = p9_allocfile(path, srv->fs);

  return 0;
}

static int rwrite(struct p9_server *srv, struct p9_req *req) {
  struct p9_fid* fid;

  if ((fid = p9_lookupfid(srv->fpool, req->ifcall.fid)) == 0) {
    err(req, P9_UNKNOWNFID);
    return 0;
  }
  
  if (fid->fd == -1 && (fid->fd = p9open(fid->file->path, O_RDWR)) < 0) {
    err(req, P9_NOFILE);
    return 0;
  }

  if (to_offset(fid, req->ifcall.offset) == -1) {
    err(req, P9_NOFILE);
    return 0;
  }

  int wsize;
  if ((wsize = write(fid->fd, req->ifcall.data, req->ifcall.count)) <= 0) {
    err(req, P9_BOTCH);
    return 0;
  }
  fid->offset += wsize;
  req->ofcall.count = req->ifcall.count;

  return 0;
}

static int rremove(struct p9_server *srv, struct p9_req *req) {
  struct p9_fid *fid;
  if ((fid = p9_lookupfid(srv->fpool, req->ifcall.fid)) == 0) {
    err(req, P9_UNKNOWNFID);
    return 0;
  }
  if (unlink(fid->file->path) == -1) {
    err(req, P9_NOFILE);
  }
  srv->fpool->destroy(fid);
  return 0;
}

static int rclunk(struct p9_server *srv, struct p9_req *req) {
  struct p9_fid *fid;
  if ((fid = p9_removefid(srv->fpool, req->ifcall.fid)) == 0) {
    err(req, P9_UNKNOWNFID);
    return 0;
  }

  srv->fpool->destroy(fid);
  return 0;
}

static int rflush(struct p9_server *srv, struct p9_req *req) {
  srv->done = 1;
  return 0;
}

static int rstat(struct p9_server *srv, struct p9_req *req) {
  struct p9_fid *fid;

  if ((fid = p9_lookupfid(srv->fpool, req->ifcall.fid)) == 0) {
    err(req, P9_UNKNOWNFID);
    return 0;
  }

  struct p9_stat* stat;
  
  if ((stat = p9_getstat(fid->file->path)) == 0) {
    err(req, P9_NOFILE);
    return 0;
  }
  
  req->ofcall.stat = stat;
  req->ofcall.nstat = stat->size;
  req->ofcall.parlen = stat->size+BIT16SZ;

  return 0;
}

static int rwstat(struct p9_server* srv, struct p9_req* req) {
  return 0;
}

static int rignore(struct p9_server* srv, struct p9_req* req) {
  return -1;
}

static int (*rfuncs[]) (struct p9_server* srv, struct p9_req* req) = {
  [P9_TVERSION]     rversion,
  [P9_TAUTH]        rignore,
  [P9_TATTACH]      rattach,
  [P9_TERROR]       rignore,
  [P9_TFLUSH]       rflush,
  [P9_TWALK]        rwalk,
  [P9_TOPEN]        ropen,
  [P9_TCREATE]      rcreate,
  [P9_TREAD]        rread,
  [P9_TWRITE]       rwrite,
  [P9_TCLUNK]       rclunk,
  [P9_TREMOVE]      rremove,
  [P9_TSTAT]        rstat,
  [P9_TWSTAT]       rwstat
};

static int start_server(struct p9_server *srv) {
  srv->msize = P9_MAXMSGLEN;
  srv->conn.sockfd = socket(SOCK_TCP);
  if (srv->conn.sockfd == -1) {
    printf("socket error!\n");
    return -1;
  }
  if (listen(srv->conn.sockfd, P9_PORT) == -1) {
    printf("socket error!\n");
    return -1;
  }
  return 0;
}

static void stop_server(struct p9_server *srv) {
  close(srv->conn.sockfd);
  srv->conn.sockfd = 0;
  free(srv->conn.wbuf);
  free(srv->conn.rbuf);
  free(srv->fs);
  p9_freefidpool(srv->fpool);
}

static void initserver(struct p9_server *srv) {
  srv->done = 0;
  srv->debug = 1;
  srv->msize = P9_MAXMSGLEN;
  srv->conn.wbuf = p9malloc(srv->msize);
  srv->conn.rbuf = p9malloc(srv->msize);
  srv->fpool = p9_allocfidpool();
  srv->fs = p9malloc(sizeof(struct p9_filesystem));
  srv->fs->rootpath = "/";
  srv->fs->rootpathlen = 1;
  srv->fs->root = p9_allocfile(srv->fs->rootpath, srv->fs);
  srv->start = start_server;
  srv->stop = stop_server;
  srv->send = p9_sendreq;
  srv->recv = p9_recvreq;
  srv->rfuncs = rfuncs;
}

int main(int argc, char **argv) {
  printf("Launch 9p server!\n");
  struct p9_server srv;
  initserver(&srv);

  if (srv.start(&srv) == -1) {
    goto fail;
  }

  struct p9_req *req;
  while ((req = srv.recv(&srv.conn)) != 0) {
    if (srv.debug)
      p9_debugfcall(&req->ifcall);
    if (srv.rfuncs[req->ifcall.type](&srv, req) == -1) {
      goto fail;
    }
    respond(&srv, req);
    if (srv.debug)
      p9_debugfcall(&req->ofcall);
    p9_freereq(req);
    req = 0;

    if (srv.done)
      break;
  }
  srv.stop(&srv);
  exit(0);

fail:
  srv.stop(&srv);
  exit(1);
}

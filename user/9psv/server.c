#include "user.h"
#include "fcntl.h"
#include "net/byteorder.h"
#include "net/socket.h"
#include "p9.h"
#include "p9_server.h"
#include "errno.h"

static int respond(struct p9_server *srv, struct p9_req *req) {
  if (!req->error) {
    req->ofcall.type = req->ifcall.type+1;
  } else {
    req->ofcall.type = P9_RERROR;
  }
  req->ofcall.size = p9_getfcallsize(&req->ofcall);
  req->ofcall.tag = req->ifcall.tag;

  // TODO error processing

  if (p9_composefcall(&req->ofcall, srv->conn.wbuf, srv->msize) == -1 ) {
    return -1;
  }

  if (srv->send(&srv->conn, req) == -1) {
    return -1;
  }
  return 0;
}

static int rversion(struct p9_server *srv, struct p9_req *req) {
  if (strncmp(req->ifcall.version, VERSION9P, strlen(VERSION9P)) != 0) {
    req->ofcall.version = "unknown";
    req->ofcall.msize = req->ifcall.msize;
    return -1;
  }
  req->ofcall.version = VERSION9P;
  req->ofcall.msize = req->ifcall.msize;
  return 0;
}

static int rattach(struct p9_server *srv, struct p9_req *req) {
  struct p9_qid* root = srv->fs->root;
  if ((req->fid = p9_allocfid(srv->fpool, req->ifcall.fid, root)) == 0) {
    // TODO: error respond
    printf("cannot allocate fid\n");
    return -1;
  }
  // We don't support afid at present. Afid is a special fid provided to prove
  // service has a permission to attach.
  if (req->ifcall.afid != P9_NOFID) {
    // TODO: lookup afid and respond error when there is no afid
    req->error = 1;
    req->ofcall.ecode = EPERM;
    return -1;
  }

  req->ofcall.qid = root;
  return 0;
}

static int rwalk(struct p9_server *srv, struct p9_req *req) {
  char path[256], *p;
  struct p9_qid *par;
  struct p9_fid *fid;

  if ((fid = p9_lookupfid(srv->fpool, req->ifcall.fid)) == 0) {
    req->error = 1;
    req->ofcall.ecode = EIO;
    return 0;
  }
  par = fid->qid;

  strcpy(path, par->pathname);
  p = path+strlen(par->pathname);
  for (uint32_t i = 0; i < req->ifcall.nwname; i++) {
    struct p9_qid *qid;
    strcpy(p, req->ifcall.wname[i]);
    p += strlen(req->ifcall.wname[i]);
    int qpath = p9_getqidno(path);
    if (qpath == -1) {
      req->error = 1;
      req->ofcall.ecode = ENOENT;
      return 0;
    }
    if ((qid = p9_lookupqid(srv->qpool, qpath)) == 0) {
      qid = p9_allocqid(srv->qpool, par, srv->fs, path);
    }
    if (qid == 0) {
      req->error = 1;
      req->ofcall.ecode = ENOENT;
      return 0;
    } 
    par = qid;
    req->ofcall.wqid[i] = qid;
  }
  req->ofcall.nwqid = req->ifcall.nwname;
  if ((req->fid = p9_allocfid(
        srv->fpool,
        req->ifcall.newfid,
        par)) == 0
  ) {
    printf("[rwalk] cannot allocate newfid\n");
    return -1;
  }
  return 0;
}

static int ropen(struct p9_server *srv, struct p9_req *req) {
  struct p9_fid *fid;
  if ((fid = p9_lookupfid(srv->fpool, req->ifcall.fid)) == 0) {
    req->error = 1;
    req->ofcall.ecode = ENOENT;
    return 0;
  }
  
  if (fid->qid == 0) {
    req->error = 1;
    req->ofcall.ecode = ENOENT;
    return 0;
  }

  struct p9_file* file = fid->qid->file;
  if (file == 0) {
    req->error = 1;
    req->ofcall.ecode = ENOENT;
    return 0;
  }

  int mode = P9_IS_DIR(fid->qid->type) ? O_RDONLY : O_RDWR;
  if ((file->fd = open(file->path, mode)) == -1) {
    req->error = 1;
    req->ofcall.ecode = ENOENT;
    return 0;
  }

  req->ofcall.qid = fid->qid;

  // TODO: mode change(req->ifcall.mode)
  return 0;
}

static int rclunk(struct p9_server *srv, struct p9_req *req) {
  struct p9_fid *fid;
  if ((fid = p9_removefid(srv->fpool, req->ifcall.fid)) == 0) {
    req->error = 1;
    req->ofcall.ecode = EIO;
    return 0;
  }
  struct p9_file* file = fid->qid->file;
  if (file->fd != -1) {
    close(file->fd);
    file->fd = -1;
  }
  srv->fpool->destroy(fid);
  return 0;
}

static int rread(struct p9_server *srv, struct p9_req *req) {
  struct p9_fid *fid;
  struct p9_qid *qid;
  if ((fid = p9_lookupfid(srv->fpool, req->ifcall.fid)) == 0) {
    req->error = 1;
    req->ofcall.ecode = EIO;
    return 0;
  }
  int count = req->ifcall.count >= P9_MAXMSGLEN ? 4000 : req->ifcall.count;
  req->ofcall.data = malloc(count);

  qid = fid->qid;
  if (P9_IS_DIR(qid->type)) {
    if (req->ifcall.offset > 0) {
      req->ofcall.count = 0;
      return 0;
    }
    // TODO: offset
    // TODO: dir update
    p9_get_dir(qid, srv->fs);
    char* dp = req->ofcall.data;
    int sum = 0;
    struct p9_file* file = qid->file;
    for (int i = 0; i < file->child_num; i++) {
      struct p9_qid*  chqid   = file->childs[i];
      struct p9_stat* chstat    = p9_get_stat(chqid->pathname);
      p9_compose_stat(dp, chstat, chqid);
      dp += chstat->size+BIT16SZ;
      sum += chstat->size+BIT16SZ;
      free(chstat);
    }
    req->ofcall.count = sum;
  // file
  } else {
    // TODO: offset process
    struct p9_file* file = qid->file;
    if (file->fd == -1) {
      req->error = 1;
      req->ofcall.ecode = ENOENT;
      return 0;
    }
    if ((req->ofcall.count = read(file->fd, req->ofcall.data, count)) < 0) {
      req->error = 1;
      req->ofcall.ecode = EIO;
      return 0;
    }
  }

  return 0;
}

static int rwrite(struct p9_server *srv, struct p9_req *req) {
  req->error = 1;
  req->ofcall.ecode = ENOSYS;
  return 0;
}

static int rstat(struct p9_server *srv, struct p9_req *req) {
  struct p9_fid *fid;

  if ((fid = p9_lookupfid(srv->fpool, req->ifcall.fid)) == 0) {
    req->error = 1;
    req->ofcall.ecode = EIO;
    return 0;
  }

  struct p9_file* file = fid->qid->file;
  struct p9_stat* stat = p9_get_stat(file->path);
  
  if (stat->size < 0) {
    req->error = 1;
    req->ofcall.ecode = ENOSYS;
    return 0;
  }

  req->ofcall.stat = stat;
  req->ofcall.nstat = stat->size;
  req->ofcall.parlen = stat->size+BIT16SZ;
  req->ofcall.statqid = fid->qid;
  return 0;
}

static int rremove(struct p9_server *srv, struct p9_req *req) {
  struct p9_fid *fid;
  struct p9_qid *qid;
  if ((fid = p9_lookupfid(srv->fpool, req->ifcall.fid)) == 0) {
    req->error = 1;
    req->ofcall.ecode = EIO;
    return 0;
  }
  qid = fid->qid;
  if (unlink(qid->pathname) == -1) {
    req->error = 1;
    req->ofcall.ecode = EIO;
  }
  srv->fpool->destroy(fid);
  srv->qpool->destroy(qid);
  return 0;
}

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
  p9_freeqidpool(srv->qpool);
}

static void initserver(struct p9_server *srv) {
  srv->msize = P9_MAXMSGLEN;
  srv->conn.wbuf = malloc(srv->msize);
  srv->conn.rbuf = malloc(srv->msize);
  srv->fpool = p9_allocfidpool();
  srv->qpool = p9_allocqidpool();
  srv->fs = malloc(sizeof(struct p9_filesystem));
  srv->fs->rootpath = "/";
  srv->fs->rootpathlen = 1;
  srv->fs->root = p9_allocqid(srv->qpool, 0, srv->fs, srv->fs->rootpath);
  srv->start = start_server;
  srv->stop = stop_server;
  srv->send = p9_sendreq;
  srv->recv = p9_recvreq;
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
    p9_debugfcall(&req->ifcall);
    switch (req->ifcall.type) {
      case P9_TLERROR:
        goto fail;
        break;
      case P9_TVERSION:
        if (rversion(&srv, req) == -1) {
          goto fail;
        }
        break;
      case P9_TAUTH:
        goto fail;
        break;
      case P9_TATTACH:
        if (rattach(&srv, req) == -1) {
          goto fail;
        }
        break;
      case P9_TWALK:
        if (rwalk(&srv, req) == -1) {
          goto fail;
        }
        break;
      case P9_TOPEN:
        if (ropen(&srv, req) == -1) {
          goto fail;
        }
        break;
      case P9_TFLUSH:
        break;
      case P9_TCREATE:
        break;
      case P9_TREAD:
        if (rread(&srv, req) == -1) {
          goto fail;
        }
        break;
      case P9_TWRITE:
        if (rwrite(&srv, req) == -1) {
          goto fail;
        }
        break;
      case P9_TCLUNK:
        if (rclunk(&srv, req) == -1) {
          goto fail;
        }
        break;
      case P9_TREMOVE:
        if (rremove(&srv, req) == -1) {
          goto fail;
        }
        break;
      case P9_TSTAT:
        if (rstat(&srv, req) == -1) {
          goto fail;
        }
        break;
      case P9_TWSTAT:
        break;
      default:
        goto fail;
    }
    respond(&srv, req);
    p9_debugfcall(&req->ofcall);
    p9_freereq(req);
    req = 0;
  }
  srv.stop(&srv);
  exit(0);

fail:
  srv.stop(&srv);
  exit(1);
}

#include "user.h"
#include "types.h"
#include "stat.h"
#include "arch/riscv.h"
#include "param.h"
#include "fcntl.h"
#include "net/byteorder.h"
#include "net/socket.h"
#include "styx2000.h"
#include "fcall.h"
#include "styx2000_server.h"

static int respond(struct styx2000_server *srv, struct styx2000_req *req) {
  if (!req->error) {
    req->ofcall.type = req->ifcall.type+1;
  } else {
    req->ofcall.type = STYX2000_RERROR;
  }
  req->ofcall.size = styx2000_getfcallsize(&req->ofcall);
  req->ofcall.tag = req->ifcall.tag;

  // TODO error processing

  if (styx2000_composefcall(&req->ofcall, srv->conn.wbuf, srv->msize) == -1 ) {
    return -1;
  }

  if (srv->send(&srv->conn, req) == -1) {
    return -1;
  }
  return 0;
}

static int rversion(struct styx2000_server *srv, struct styx2000_req *req) {
  if (strncmp(req->ifcall.version, "9P2000", 6) != 0) {
    req->ofcall.version = "unknown";
    req->ofcall.msize = req->ifcall.msize;
    return -1;
  }
  req->ofcall.version = "9P2000";
  req->ofcall.msize = req->ifcall.msize;
  return 0;
}

static int rattach(struct styx2000_server *srv, struct styx2000_req *req) {
  struct styx2000_qid* root = srv->fs->root;
  if ((req->fid = styx2000_allocfid(srv->fpool, req->ifcall.fid, root)) == 0) {
    // TODO: error respond
    printf("cannot allocate fid\n");
    return -1;
  }
  // We don't support afid at present. Afid is a special fid provided to prove
  // service has a permission to attach.
  if (req->ifcall.afid != STYX2000_NOFID) {
    // TODO: lookup afid and respond error when there is no afid
    req->error = 1;
    req->ofcall.ename = "not supported afid";
    return -1;
  }

  req->ofcall.qid = root;
  return 0;
}

static int rwalk(struct styx2000_server *srv, struct styx2000_req *req) {
  char path[256], *p;
  struct styx2000_qid *par;
  struct styx2000_fid *fid;

  if ((fid = styx2000_lookupfid(srv->fpool, req->ifcall.fid)) == 0) {
    req->error = 1;
    req->ofcall.ename = "not allocated fid";
    return 0;
  }
  par = fid->qid;

  strcpy(path, par->pathname);
  p = path+strlen(par->pathname);
  for (int i = 0; i < req->ifcall.nwname; i++) {
    struct styx2000_qid *qid;
    strcpy(p, req->ifcall.wname[i]);
    p += strlen(req->ifcall.wname[i]);
    int qpath = styx2000_getqidno(path);
    if (qpath == -1) {
      req->error = 1;
      req->ofcall.ename = "cannot open path";
      return 0;
    }
    if ((qid = styx2000_lookupqid(srv->qpool, qpath)) == 0) {
      qid = styx2000_allocqid(srv->qpool, par, srv->fs, path);
    }
    if (qid == 0) {
      req->error = 1;
      req->ofcall.ename = "cannot open path";
      return 0;
    } 
    par = qid;
    req->ofcall.wqid[i] = qid;
  }
  req->ofcall.nwqid = req->ifcall.nwname;
  if ((req->fid = styx2000_allocfid(
        srv->fpool,
        req->ifcall.newfid,
        par)) == 0
  ) {
    fprintf(2, "[rwalk] cannot allocate newfid\n");
    return -1;
  }
  return 0;
}

static int ropen(struct styx2000_server *srv, struct styx2000_req *req) {
  struct styx2000_fid *fid;
  if ((fid = styx2000_lookupfid(srv->fpool, req->ifcall.fid)) == 0) {
    req->error = 1;
    req->ofcall.ename = "specified fid was not allocated.";
    return 0;
  }
  
  if (fid->qid == 0) {
    req->error = 1;
    req->ofcall.ename = "there are no specified file";
    return 0;
  }

  struct styx2000_file* file = fid->qid->file;
  if (file == 0) {
    req->error = 1;
    req->ofcall.ename = "specified file was not opend";
    return 0;
  }

  int mode = styx2000_is_dir(fid->qid->type) ? O_RDONLY : O_RDWR;
  if ((file->fd = open(file->path, mode)) == -1) {
    req->error = 1;
    req->ofcall.ename = "specified file cannot open";
    return 0;
  }

  req->ofcall.qid = fid->qid;

  // TODO: mode change(req->ifcall.mode)
  return 0;
}

static int rclunk(struct styx2000_server *srv, struct styx2000_req *req) {
  struct styx2000_fid *fid;
  if ((fid = styx2000_removefid(srv->fpool, req->ifcall.fid)) == 0) {
    req->error = 1;
    req->ofcall.ename = "specified fid was not allocated.";
    return 0;
  }
  struct styx2000_file* file = fid->qid->file;
  if (file->fd != -1) {
    close(file->fd);
    file->fd = -1;
  }
  srv->fpool->destroy(fid);
  return 0;
}

static int rread(struct styx2000_server *srv, struct styx2000_req *req) {
  struct styx2000_fid *fid;
  struct styx2000_qid *qid;
  if ((fid = styx2000_lookupfid(srv->fpool, req->ifcall.fid)) == 0) {
    req->error = 1;
    req->ofcall.ename = "specified fid was not allocated.";
    return 0;
  }
  int count = req->ifcall.count >= STYX2000_MAXMSGLEN ? 4000 : req->ifcall.count;
  req->ofcall.data = malloc(count);

  qid = fid->qid;
  if (styx2000_is_dir(qid->type)) {
    if (req->ifcall.offset > 0) {
      req->ofcall.count = 0;
      return 0;
    }
    // TODO: offset
    // TODO: dir update
    styx2000_get_dir(qid, srv->fs);
    char* dp = req->ofcall.data;
    int sum = 0;
    struct styx2000_file* file = qid->file;
    for (int i = 0; i < file->child_num; i++) {
      struct styx2000_qid*  chqid   = file->childs[i];
      struct styx2000_stat* chstat    = styx2000_get_stat(chqid->pathname);
      styx2000_compose_stat(dp, chstat, chqid);
      dp += chstat->size+BIT16SZ;
      sum += chstat->size+BIT16SZ;
      free(chstat);
    }
    req->ofcall.count = sum;
  // file
  } else {
    // TODO: offset process
    struct styx2000_file* file = qid->file;
    if (file->fd == -1) {
      req->error = 1;
      req->ofcall.ename = "specified file is not opend.";
      return 0;
    }
    if ((req->ofcall.count = read(file->fd, req->ofcall.data, count)) < 0) {
      req->error = 1;
      req->ofcall.ename = "cannot read file.";
      return 0;
    }
  }

  return 0;
}

static int rstat(struct styx2000_server *srv, struct styx2000_req *req) {
  struct styx2000_fid *fid;

  if ((fid = styx2000_lookupfid(srv->fpool, req->ifcall.fid)) == 0) {
    req->error = 1;
    req->ofcall.ename = "specified fid was not allocated.";
    return 0;
  }

  struct styx2000_file* file = fid->qid->file;
  struct styx2000_stat* stat = styx2000_get_stat(file->path);
  
  if (stat->size < 0) {
    req->error = 1;
    req->ofcall.ename = "make_stat error.";
    return 0;
  }

  req->ofcall.stat = stat;
  req->ofcall.nstat = stat->size;
  req->ofcall.parlen = stat->size+BIT16SZ;
  req->ofcall.statqid = fid->qid;
  return 0;
}

static int rremove(struct styx2000_server *srv, struct styx2000_req *req) {
  struct styx2000_fid *fid;
  struct styx2000_qid *qid;
  if ((fid = styx2000_lookupfid(srv->fpool, req->ifcall.fid)) == 0) {
    req->error = 1;
    req->ofcall.ename = "there are no specified file";
    return 0;
  }
  qid = fid->qid;
  if (unlink(qid->pathname) == -1) {
    req->error = 1;
    req->ofcall.ename = "cannot remove file";
  }
  srv->fpool->destroy(fid);
  srv->qpool->destroy(qid);
  return 0;
}

static int start_server(struct styx2000_server *srv);
static void stop_server(struct styx2000_server *srv);
static void initserver(struct styx2000_server *srv);

int main(int argc, char **argv) {
  printf("Launch 9p server!\n");
  struct styx2000_server srv;
  initserver(&srv);

  if (srv.start(&srv) == -1) {
    goto fail;
  }

  struct styx2000_req *req;
  while ((req = srv.recv(&srv.conn)) != 0) {
    styx2000_debugfcall(&req->ifcall);
    switch (req->ifcall.type) {
      case STYX2000_TVERSION:
        if (rversion(&srv, req) == -1) {
          goto fail;
        }
        break;
      case STYX2000_TAUTH:
        goto fail;
        break;
      case STYX2000_TATTACH:
        if (rattach(&srv, req) == -1) {
          goto fail;
        }
        break;
      case STYX2000_TWALK:
        if (rwalk(&srv, req) == -1) {
          goto fail;
        }
        break;
      case STYX2000_TOPEN:
        if (ropen(&srv, req) == -1) {
          goto fail;
        }
        break;
      case STYX2000_TFLUSH:
        break;
      case STYX2000_TCREATE:
        break;
      case STYX2000_TREAD:
        if (rread(&srv, req) == -1) {
          goto fail;
        }
        break;
      case STYX2000_TWRITE:
        break;
      case STYX2000_TCLUNK:
        if (rclunk(&srv, req) == -1) {
          goto fail;
        }
        break;
      case STYX2000_TREMOVE:
        if (rremove(&srv, req) == -1) {
          goto fail;
        }
        break;
      case STYX2000_TSTAT:
        if (rstat(&srv, req) == -1) {
          goto fail;
        }
        break;
      case STYX2000_TWSTAT:
        break;
      default:
        goto fail;
    }
    respond(&srv, req);
    styx2000_debugfcall(&req->ofcall);
    styx2000_freereq(req);
    req = 0;
  }
  srv.stop(&srv);
  exit(0);

fail:
  srv.stop(&srv);
  exit(1);
}

static int start_server(struct styx2000_server *srv) {
  srv->msize = STYX2000_MAXMSGLEN;
  srv->conn.sockfd = socket(SOCK_TCP);
  if (srv->conn.sockfd == -1) {
    printf("socket error!\n");
    return -1;
  }
  if (listen(srv->conn.sockfd, STYX2000_PORT) == -1) {
    printf("socket error!\n");
    return -1;
  }
  return 0;
}

static void stop_server(struct styx2000_server *srv) {
  close(srv->conn.sockfd);
  srv->conn.sockfd = 0;
  free(srv->conn.wbuf);
  free(srv->conn.rbuf);
  free(srv->fs);
  styx2000_freefidpool(srv->fpool);
  styx2000_freeqidpool(srv->qpool);
}

static void initserver(struct styx2000_server *srv) {
  srv->msize = STYX2000_MAXMSGLEN;
  srv->conn.wbuf = malloc(srv->msize);
  srv->conn.rbuf = malloc(srv->msize);
  srv->fpool = styx2000_allocfidpool();
  srv->qpool = styx2000_allocqidpool();
  srv->fs = malloc(sizeof(struct styx2000_filesystem));
  srv->fs->rootpath = "/";
  srv->fs->rootpathlen = 1;
  srv->fs->root = styx2000_allocqid(srv->qpool, 0, srv->fs, srv->fs->rootpath);
  srv->start = start_server;
  srv->stop = stop_server;
  srv->send = styx2000_sendreq;
  srv->recv = styx2000_recvreq;
}

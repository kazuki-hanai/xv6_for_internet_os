#include "user.h"
#include "types.h"
#include "stat.h"
#include "arch/riscv.h"
#include "param.h"
#include "net/byteorder.h"
#include "net/socket.h"
#include "styx2000.h"

static char* get_qid(char* path, struct styx2000_qid *qid) {
  int fd;
  struct stat st;

  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "cannot open path: %s\n", path);
    return "cannot open path\n";
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "cannot stat path: %s\n", path);
    close(fd);
    return "cannot stat path\n";
  }

  qid->type = st.type;
  qid->vers = 0;
  qid->path = st.ino;

  close(fd);
  return 0;
}

static int respond(struct styx2000_server *srv, struct styx2000_req *req) {
  if (!req->error) {
    req->ofcall.type = req->ifcall.type+1;
  } else {
    req->ofcall.type = STYX2000_RERROR;
  }
  req->ofcall.size = styx2000_getfcallsize(&req->ofcall);
  req->ofcall.tag = req->ifcall.tag;

  // TODO error processing

  if (styx2000_composefcall(req, srv->wbuf, srv->msize) == -1 ) {
    return -1;
  }

  if (srv->send(srv, req) == -1) {
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
  if ((req->fid = styx2000_allocfid(srv->fpool, srv->fs.rootpath, req->ifcall.fid)) == 0) {
    // TODO: error respond
    printf("cannot allocate fid\n");
    return -1;
  }
  // We don't support afid at present. Afid is a special fid provided to prove
  // service has a permission to attach.
  if (req->ifcall.afid != STYX2000_NOFID) {
    // TODO: lookup afid and respond error when there is no afid
    return -1;
  }

  if ((req->ofcall.ename = get_qid(srv->fs.rootpath, &req->ofcall.qid)) != 0) {
      req->error = 1;
  }
  return 0;
}

static int rwalk(struct styx2000_server *srv, struct styx2000_req *req) {
  char path[256];
  char *p = path;
  strcpy(p, srv->fs.rootpath);
  p += strlen(srv->fs.rootpath);
  for (int i = 0; i < req->ifcall.nwname; i++) {
    strcpy(p, req->ifcall.wname[i]);
    if ((req->ofcall.ename = get_qid(p, &req->ofcall.wqid[i])) != 0) {
      req->error = 1;
      return 0;
    }
    p += strlen(req->ifcall.wname[i]);
  }
  if ((req->fid = styx2000_allocfid(srv->fpool, path, req->ifcall.newfid)) == 0) {
    printf("[rwalk] cannot allocate newfid\n");
    return -1;
  }
  return 0;
}

static int start_server(struct styx2000_server *srv) {
  srv->msize = STYX2000_MAXMSGLEN;
  srv->sockfd = socket(SOCK_TCP);
  if (srv->sockfd == -1) {
    printf("socket error!\n");
    return -1;
  }
  if (listen(srv->sockfd, STYX2000_PORT) == -1) {
    printf("socket error!\n");
    return -1;
  }
  return 0;
}

static void stop_server(struct styx2000_server *srv) {
  close(srv->sockfd);
  free(srv->wbuf);
  free(srv->rbuf);
  srv->sockfd = 0;
}

static void initserver(struct styx2000_server *srv) {
  srv->msize = STYX2000_MAXMSGLEN;
  srv->wbuf = malloc(srv->msize);
  srv->rbuf = malloc(srv->msize);
  srv->fs.rootpath = "/";
  srv->fpool = styx2000_allocfidpool();
  srv->start = start_server;
  srv->stop = stop_server;
  srv->send = styx2000_sendreq;
  srv->recv = styx2000_recvreq;
}

int main(int argc, char **argv) {
  printf("Launch 9p server!\n");
  struct styx2000_server srv;
  initserver(&srv);

  if (srv.start(&srv) == -1) {
    goto fail;
  }

  struct styx2000_req *req;
  while ((req = srv.recv(&srv)) != 0) {
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
      case STYX2000_TFLUSH:
        break;
      case STYX2000_TOPEN:
        break;
      case STYX2000_TCREATE:
        break;
      case STYX2000_TREAD:
        break;
      case STYX2000_TWRITE:
        break;
      case STYX2000_TCLUNK:
        break;
      case STYX2000_TREMOVE:
        break;
      case STYX2000_TSTAT:
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

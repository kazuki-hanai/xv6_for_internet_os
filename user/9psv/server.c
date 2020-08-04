#include "user.h"
#include "types.h"
#include "stat.h"
#include "arch/riscv.h"
#include "param.h"
#include "net/byteorder.h"
#include "net/socket.h"
#include "styx2000.h"

static int get_qid(char* path, struct styx2000_qid *qid) {
  int fd;
  struct stat st;

  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "cannot open path: %s\n", path);
    return -1;
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "cannot stat path: %s\n", path);
    close(fd);
    return -1;
  }

  qid->type = st.type;
  qid->vers = 0;
  qid->path = st.ino;

  close(fd);
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

  get_qid(srv->fs.rootpath, &req->ofcall.qid);
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
  if (fork() != 0)
    exit(0);
  printf("Launch 9p server!\n");
  struct styx2000_server srv;
  initserver(&srv);

  if (srv.start(&srv) == -1) {
    goto fail;
  }

  struct styx2000_req *req;
  while ((req = srv.recv(&srv)) != 0) {
    switch (req->ifcall.type) {
      case STYX2000_TVERSION:
        printf("TVERSION: %s\n", req->ifcall.version);
        if (rversion(&srv, req) == -1) {
          goto fail;
        }
        break;
      case STYX2000_RVERSION:
        goto fail;
        break;
      case STYX2000_TAUTH:
        goto fail;
        break;
      case STYX2000_RAUTH:
        goto fail;
        break;
      case STYX2000_TATTACH:
        printf("TATTACH: fid: %d, afid: %d, uname: %s, aname: %s\n",
          req->ifcall.fid, req->ifcall.afid, req->ifcall.uname, req->ifcall.aname);
        if (rattach(&srv, req) == -1) {
          goto fail;
        }
        printf("RATTACH: qid { type: %d, vers: %d, path: %d }\n",
          req->ofcall.qid.type, req->ofcall.qid.vers, req->ofcall.qid.path);
        break;
      case STYX2000_RATTACH:
        break;
      case STYX2000_RERROR:
        break;
      case STYX2000_TFLUSH:
        break;
      case STYX2000_RFLUSH:
        break;
      case STYX2000_TWALK:
        break;
      case STYX2000_RWALK:
        break;
      case STYX2000_TOPEN:
        break;
      case STYX2000_ROPEN:
        break;
      case STYX2000_TCREATE:
        break;
      case STYX2000_RCREATE:
        break;
      case STYX2000_TREAD:
        break;
      case STYX2000_RREAD:
        break;
      case STYX2000_TWRITE:
        break;
      case STYX2000_RWRITE:
        break;
      case STYX2000_TCLUNK:
        break;
      case STYX2000_RCLUNK:
        break;
      case STYX2000_TREMOVE:
        break;
      case STYX2000_RREMOVE:
        break;
      case STYX2000_TSTAT:
        break;
      case STYX2000_RSTAT:
        break;
      case STYX2000_TWSTAT:
        break;
      case STYX2000_RWSTAT:
        break;
    }
    styx2000_respond(&srv, req);
    styx2000_freereq(req);
    req = 0;
  }
  if (req == 0) {
    goto fail;
  }
  srv.stop(&srv);
  exit(0);

fail:
  srv.stop(&srv);
  exit(1);
}

#include "user.h"
#include "types.h"
#include "arch/riscv.h"
#include "param.h"
#include "net/byteorder.h"
#include "net/socket.h"
#include "net/styx2000.h"

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
  srv->start = start_server;
  srv->stop = stop_server;
  srv->send = styx2000_sendreq;
  srv->recv = styx2000_recvreq;
}

int main(int argc, char **argv) {
  if (fork() != 0)
    exit(0);
  printf("start 9p server!\n");
  struct styx2000_server srv;
  initserver(&srv);

  if (srv.start(&srv) == -1) {
    goto fail;
  }

  struct styx2000_req *req;
  while ((req = srv.recv(&srv)) != 0) {
    switch (req->ifcall.type) {
      case STYX2000_TVERSION:
        if (styx2000_tversion(&srv, req) == -1) {
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
        if (styx2000_tattach(&srv, req) == -1) {
          goto fail;
        }
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

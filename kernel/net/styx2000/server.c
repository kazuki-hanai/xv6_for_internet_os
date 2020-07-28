#include "types.h"
#include "arch/riscv.h"
#include "defs.h"
#include "param.h"
#include "net/byteorder.h"
#include "net/socket.h"
#include "net/styx2000.h"

void styx2000_initserver(struct styx2000_server *server) {

}

int styx2000_serve() {
  struct sock_cb *scb = sockalloc(SOCK_TCP);

  if (socklisten(scb, STYX2000_PORT) == -1) {
    return -1;
  }

  struct styx2000_server server;
  styx2000_initserver(&server);

  uint8 rbuf[2048];
  // uint8 wbuf[2048];
  int rsize;
  // int wsize;
  while ((rsize = sockrecv(scb, (uint64)rbuf, sizeof(rbuf), 0)) != -1) {
    struct styx2000_message *message;
    message = styx2000_parsecall(rbuf, rsize);
    switch (message->type) {
      case STYX2000_TVERSION:
        printf("type: %s\n", message->fcall->get_message(message));
        break;
      case STYX2000_RVERSION:
        break;
      case STYX2000_TAUTH:
        return 0;
        break;
      case STYX2000_RAUTH:
        return 0;
        break;
      case STYX2000_TATTACH:
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
    bd_free(message);
  }
  
  sockclose(scb);
  return 0;
}

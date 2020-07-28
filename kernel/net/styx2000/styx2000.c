#include "types.h"
#include "arch/riscv.h"
#include "defs.h"
#include "param.h"
#include "net/styx2000.h"
#include "net/byteorder.h"

extern struct styx2000_fcall styx2000_tversion_fcall;
extern struct styx2000_fcall styx2000_rversion_fcall;

struct styx2000_message* styx2000_parsecall(uint8* buf, int size) {
  if (buf == 0) {
    return 0;
  }
  if (size < sizeof(struct styx2000_header)) {
    return 0;
  }

  struct styx2000_header *hdr = (struct styx2000_header *)buf;
  uint8 type = hdr->type;
  uint16 tag = ntohs(hdr->tag);

  size = ntohl(hdr->size) - sizeof(*hdr);
  buf += sizeof(*hdr);

  struct styx2000_message *message = bd_alloc(sizeof(struct styx2000_message));
  switch (type) {
    case STYX2000_TVERSION:
      if (tag != NOTAG) {
        // Error
      }
      message->type = type;
      message->size = size;
      message->buf = buf;
      message->fcall = &styx2000_tversion_fcall;
      message->fcall->parse(message);
      break;
    case STYX2000_RVERSION:
      if (tag != NOTAG) {
        // Error
      }
      message->type = type;
      message->size = size;
      message->buf = buf;
      message->fcall = &styx2000_rversion_fcall;
      message->fcall->parse(message);
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

  return message;
}

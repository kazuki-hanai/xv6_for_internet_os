#include "types.h"
#include "arch/riscv.h"
#include "defs.h"
#include "param.h"
#include "net/styx2000.h"
#include "net/byteorder.h"

struct styx2000_fcall styx2000_vesion_fcall = {
  .get_message = ,
  .compose = ,
  .parse = 
};

struct styx2000_fcall* styx2000_parsecall(uint8* raw_buf, int raw_size) {
  if (raw_buf == 0) {
    return 0;
  }
  if (raw_size < sizeof(struct styx2000_header)) {
    return 0;
  }

  struct styx2000_header *hdr = (struct styx2000_header *)raw_buf;
  uint32 size = ntohl(hdr->size);
  uint32 msg_size = size - sizeof(*hdr);
  uint8 type = hdr->type;
  uint16 tag = ntohs(hdr->tag);
  uint8 *buf = raw_buf + sizeof(*hdr);

  if (raw_size != size) {
    return 0;
  }

  void *message = 0;
  switch (type) {
    case STYX2000_TVERSION:
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

  message.parse(buf, msg_size);

  return message;
}
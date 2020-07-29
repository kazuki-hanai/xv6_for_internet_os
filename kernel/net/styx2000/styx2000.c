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

  struct styx2000_message *message = styx2000_messagealloc();
  struct styx2000_header *hdr = (struct styx2000_header *)buf;
  message->hdr.size = hdr->size;
  message->hdr.type = hdr->type;
  message->hdr.tag = hdr->tag;

  int messize = hdr->size - sizeof(*hdr);
  buf += sizeof(*hdr);

  switch (hdr->type) {
    case STYX2000_TVERSION:
      if (hdr->tag != NOTAG) {
        // Error
      }
      message->fcall = &styx2000_tversion_fcall;
      if (message->fcall->parse(message, buf, messize) == -1) {
        goto fail;
      }
      break;
    case STYX2000_RVERSION:
      if (hdr->tag != NOTAG) {
        // Error
      }
      message->fcall = &styx2000_rversion_fcall;
      if (message->fcall->parse(message, buf, messize) == -1) {
        goto fail;
      }
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

fail:
  if (message)
    styx2000_messagefree(message);
  return 0;
}

int styx2000_create_rversion(uint8** buf, uint16 tag, uint16 vsize, uint8* version) {
  struct styx2000_message *message = styx2000_messagealloc();
  message->m.trversion.msize = MAXMSGLEN;
  message->m.trversion.vsize = vsize;
  message->m.trversion.version = bd_alloc(vsize);
  memmove(message->m.trversion.version, version, vsize);

  message->hdr.size = STYX2000_HDR_SIZE + STYX2000_TRVERSION_SIZE + vsize;
  message->hdr.tag = tag;
  message->hdr.type = STYX2000_RVERSION;
  message->fcall = &styx2000_tversion_fcall;

  int res = message->fcall->compose(message, buf);
  styx2000_messagefree(message);
  return res;
}

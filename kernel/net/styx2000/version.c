#include "types.h"
#include "arch/riscv.h"
#include "defs.h"
#include "param.h"
#include "net/styx2000.h"
#include "net/byteorder.h"

static char* tversion_get_message(struct styx2000_message* message) {
  return "TVERSION";
}

static char* rversion_get_message(struct styx2000_message* message) {
  return "RVERSION";
}

static int version_compose(struct styx2000_message* message, uint8** buf) {
  *buf = bd_alloc(message->hdr.size);
  int off = 0;
  memmove((*buf)+off, (void *)&message->hdr.size, 4);
  off += 4;
  memmove((*buf)+off, (void *)&message->hdr.type, 1);
  off += 1;
  memmove((*buf)+off, (void *)&message->hdr.tag, 2);
  off += 2;
  memmove((*buf)+off, (void *)&message->m.trversion.msize, 4);
  off += 4;
  memmove((*buf)+off, (void *)&message->m.trversion.vsize, 2);
  off += 2;
  memmove((*buf)+off, (void *)message->m.trversion.version, message->m.trversion.vsize);
  off += message->m.trversion.vsize;

  return off;
}

static int version_parse(struct styx2000_message* message, uint8* buf, int size) {
  if (size < STYX2000_TRVERSION_SIZE) {
    return -1;
  }

  message->m.trversion.msize = (uint16)*buf;
  buf += 4;
  message->m.trversion.vsize = (uint16)*buf;
  buf += 2;
  
  if (size < STYX2000_TRVERSION_SIZE + message->m.trversion.vsize) {
    return -1;
  }

  message->m.trversion.version = bd_alloc(message->m.trversion.vsize+1); 
  memmove(message->m.trversion.version, buf, message->m.trversion.vsize);
  message->m.trversion.version[message->m.trversion.vsize] = 0;

  return 0;
}

struct styx2000_fcall styx2000_tversion_fcall = {
  .type = STYX2000_TVERSION,
  .get_message = tversion_get_message,
  .compose = version_compose,
  .parse = version_parse
};

struct styx2000_fcall styx2000_rversion_fcall = {
  .type = STYX2000_RVERSION,
  .get_message = rversion_get_message,
  .compose = version_compose,
  .parse = version_parse
};

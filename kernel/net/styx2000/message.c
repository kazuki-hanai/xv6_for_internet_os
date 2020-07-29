#include "types.h"
#include "arch/riscv.h"
#include "defs.h"
#include "param.h"
#include "net/styx2000.h"
#include "net/byteorder.h"

struct styx2000_message* styx2000_messagealloc() {
  struct styx2000_message *message = 0;
  message = bd_alloc(sizeof(*message));
  memset(message, 0, sizeof(*message));

  return message;
}

void styx2000_messagefree(struct styx2000_message *message) {
  switch (message->hdr.type) {
    case STYX2000_TVERSION:
    case STYX2000_RVERSION:
      bd_free(message->m.trversion.version);
      break;
    default:
      break;
  }

  bd_free(message);
}

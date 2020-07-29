#include "types.h"
#include "arch/riscv.h"
#include "defs.h"
#include "param.h"
#include "net/styx2000.h"
#include "net/byteorder.h"

static char* error_get_message(struct styx2000_message* message) {
  return "ERROR";
}

static uint8* error_compose(struct styx2000_message* message) {
  return 0;
}

static int error_parse(struct styx2000_message* message, uint8* buf, int size) {
  return 0;
}

struct styx2000_fcall styx2000_error_fcall = {
  .type = STYX2000_RERROR,
  .get_message = error_get_message,
  .compose = error_compose,
  .parse = error_parse
};

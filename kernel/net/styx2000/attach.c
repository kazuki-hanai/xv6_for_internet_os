#include "types.h"
#include "arch/riscv.h"
#include "defs.h"
#include "param.h"
#include "net/styx2000.h"
#include "net/byteorder.h"

static char* tattach_get_message(struct styx2000_message* message) {
  if (message->type == STYX2000_TATTACH) {
    return "TATTACH";
  } else {
    return "WRONG MESSAGE";
  }
}

static char* rattach_get_message(struct styx2000_message* message) {
  if (message->type == STYX2000_RATTACH) {
    return "RATTACH";
  } else {
    return "WRONG MESSAGE";
  }
}

static uint8* tattach_compose(struct styx2000_message* message) {
  return 0;
}

static uint8* rattach_compose(struct styx2000_message* message) {
  return 0;
}

static int tattach_parse(struct styx2000_message* message, uint8* buf, int size) {
  return 0;
}

static int rattach_parse(struct styx2000_message* message, uint8* buf, int size) {
  return 0;
}

struct styx2000_fcall styx2000_tattach_fcall = {
  .type = STYX2000_TATTACH,
  .get_message = tattach_get_message,
  .compose = tattach_compose,
  .parse = tattach_parse
};

struct styx2000_fcall styx2000_rattach_fcall = {
  .type = STYX2000_RATTACH,
  .get_message = rattach_get_message,
  .compose = rattach_compose,
  .parse = rattach_parse
};

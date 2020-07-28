#include "types.h"
#include "arch/riscv.h"
#include "defs.h"
#include "param.h"
#include "net/styx2000.h"
#include "net/byteorder.h"

static char* tflush_get_message(struct styx2000_message* message) {
  if (message->type == STYX2000_TFLUSH) {
    return "TFLUSH";
  } else {
    return "WRONG MESSAGE";
  }
}

static char* rflush_get_message(struct styx2000_message* message) {
  if (message->type == STYX2000_RFLUSH) {
    return "RFLUSH";
  } else {
    return "WRONG MESSAGE";
  }
}

static uint8* tflush_compose(struct styx2000_message* message) {
  return 0;
}

static uint8* rflush_compose(struct styx2000_message* message) {
  return 0;
}

static int tflush_parse(struct styx2000_message* message) {
  struct styx2000_tflush *tflush = 0;

  if (message->size < sizeof(*tflush)) {
    return -1;
  }

  tflush = (struct styx2000_tflush *)message->buf;

  return 0;
}

static int rflush_parse(struct styx2000_message* message) {
  struct styx2000_rflush *rflush = 0;

  if (message->size < sizeof(*rflush)) {
    return -1;
  }

  rflush = (struct styx2000_rflush *)message->buf;

  return 0;
}

struct styx2000_fcall styx2000_tflush_fcall = {
  .type = STYX2000_TFLUSH,
  .get_message = tflush_get_message,
  .compose = tflush_compose,
  .parse = tflush_parse
};

struct styx2000_fcall styx2000_rflush_fcall = {
  .type = STYX2000_RFLUSH,
  .get_message = rflush_get_message,
  .compose = rflush_compose,
  .parse = rflush_parse
};

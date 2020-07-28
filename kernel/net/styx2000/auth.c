#include "types.h"
#include "arch/riscv.h"
#include "defs.h"
#include "param.h"
#include "net/styx2000.h"
#include "net/byteorder.h"

static char* tauth_get_message(struct styx2000_message* message) {
  if (message->type == STYX2000_TAUTH) {
    return "TAUTH";
  } else {
    return "WRONG MESSAGE";
  }
}

static char* rauth_get_message(struct styx2000_message* message) {
  if (message->type == STYX2000_RAUTH) {
    return "RAUTH";
  } else {
    return "WRONG MESSAGE";
  }
}

static uint8* tauth_compose(struct styx2000_message* message) {
  return 0;
}

static uint8* rauth_compose(struct styx2000_message* message) {
  return 0;
}

static int tauth_parse(struct styx2000_message* message) {
  struct styx2000_tauth *tauth = 0;

  if (message->size < sizeof(*tauth)) {
    return -1;
  }

  tauth = (struct styx2000_tauth *)message->buf;

  return 0;
}

static int rauth_parse(struct styx2000_message* message) {
  struct styx2000_rauth *rauth = 0;

  if (message->size < sizeof(*rauth)) {
    return -1;
  }

  rauth = (struct styx2000_rauth *)message->buf;

  return 0;
}

struct styx2000_fcall styx2000_tauth_fcall = {
  .type = STYX2000_TAUTH,
  .get_message = tauth_get_message,
  .compose = tauth_compose,
  .parse = tauth_parse
};

struct styx2000_fcall styx2000_rauth_fcall = {
  .type = STYX2000_RAUTH,
  .get_message = rauth_get_message,
  .compose = rauth_compose,
  .parse = rauth_parse
};

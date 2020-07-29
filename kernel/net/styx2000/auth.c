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

static int tauth_parse(struct styx2000_message* message, uint8* buf, int size) {
  return 0;
}

static int rauth_parse(struct styx2000_message* message, uint8* buf, int size) {
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

#include "types.h"
#include "arch/riscv.h"
#include "defs.h"
#include "param.h"
#include "net/styx2000.h"
#include "net/byteorder.h"

static char* version_get_message(struct styx2000_message* message) {
  return "VERSION";
}

static uint8* version_compose(struct styx2000_message* message) {
  return 0;
}

static int version_parse(struct styx2000_message* message) {
  struct styx2000_trversion *version = 0;

  printf("%d, %d\n", message->size, sizeof(*version));
  if (message->size < sizeof(struct styx2000_trversion)) {
    return 0;
  }

  version = (struct styx2000_trversion *)message->buf;
  version->version = (char *)message->buf+4;

  for (int i = 0; i < message->size-4; i++) {
    printf("%x ", version->version[i]);
  }
  if (memcmp(version->version, "9P2000", 6) == 0) {

  } else if (memcmp(version->version, "9P", 2) == 0) {

  } else {
    return -1;
  }
  message->trversion.msize = ntohl(version->msize);
  message->trversion.version = version->version;
  return 0;
}

struct styx2000_fcall styx2000_tversion_fcall = {
  .type = STYX2000_TVERSION,
  .get_message = version_get_message,
  .compose = version_compose,
  .parse = version_parse
};

struct styx2000_fcall styx2000_rversion_fcall = {
  .type = STYX2000_RVERSION,
  .get_message = version_get_message,
  .compose = version_compose,
  .parse = version_parse
};

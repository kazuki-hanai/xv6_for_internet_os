#include "types.h"
#include "arch/riscv.h"
#include "defs.h"
#include "param.h"
#include "net/byteorder.h"
#include "net/socket.h"
#include "net/styx2000.h"
#include "fcall.h"

struct styx2000_req* styx2000_allocreq() {
  struct styx2000_req *req;
  req = bd_alloc(sizeof(*req));
  if (req == 0) {
    panic("[styx2000_allocreq] could not allocate");
  }
  return req;
}

void styx2000_freereq(struct styx2000_req *req) {
  bd_free(req);
}

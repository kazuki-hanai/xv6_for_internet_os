#include "types.h"
#include "defs.h"
#include "file.h"
#include "net/styx2000.h"

uint64 sys_styx2000_deamon() {
  return styx2000_serve();
}

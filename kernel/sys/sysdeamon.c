#include "types.h"
#include "defs.h"
#include "file.h"
#include "net/styx2000.h"

void sys_styx2000_deamon() {
  styx2000_serve();
}

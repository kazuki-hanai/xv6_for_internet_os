#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
  if (fork() == 0) {
    styx2000_deamon();
  } else {
  }
  exit(0);
}

#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "user.h"

int
main(int argc, char **argv)
{
  if (fork() == 0) {
    if (styx2000_deamon() == -1) {
      printf("styx2000_deamon error occured!\n");
      exit(1);
    }
    printf("shutdown server!\n");
  } else {
  }
  exit(0);
}

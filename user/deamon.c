#include "kernel/include/types.h"
#include "kernel/include/stat.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
  if (fork() == 0) {
    while(1) {
      sleep(10);
      printf("sleep...\n");
    }
  } else {
  }
  exit(0);
}

#include "user.h"

// file system tests

char buf[1024];

int
main(void)
{
  puts("userfs running\n");
  block();
  exec("usertests");
  return 0;
}

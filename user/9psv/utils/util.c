#include "user.h"

void* p9malloc(int size) {
  void* a = malloc(size);
  if (a == 0) {
    printf("cannot malloc\n");
    exit(1);
  }
  return a;
}

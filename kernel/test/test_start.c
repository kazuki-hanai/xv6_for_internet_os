#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "defs.h"
#include "test/test.h"

void test_start() {
  printf("start testing...\n\n");
  
  if (cpuid() == 0) {
    // buddy_test();
    arp_test();
    tcp_test();
    sysnet_test();
  }
}
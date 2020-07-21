#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "defs.h"

void test_start();
void buddy_test();
void sysnet_test();
void tcp_test();
void arp_test();
void sock_cb_test();

void test_start() {
  printf("start testing...\n\n");
  
  if (cpuid() == 0) {
    // buddy_test();
    arp_test();
    sock_cb_test();
    sysnet_test();
    tcp_test();
  }
  printf("test done!!!\n\n");
}
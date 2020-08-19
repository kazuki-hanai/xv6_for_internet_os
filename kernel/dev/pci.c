#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "memlayout.h"

void pci_init()
{

  for(int dev = 0; dev < 32; dev++) {
    int bus = 0;
    int func = 0;
    int offset = 0;
    uint32_t off = (bus << 16) | (dev << 11) | (func << 8) | (offset);
    volatile uint32_t *base = (uint32_t *)ECAM + off;
    uint32_t id = base[0];

    if (id != -1) 
      printf("pci id: %x\n", id);

    if(id == 0x100e8086) {
      base[1] = 7;
      __sync_synchronize();

      // for(int i = 0; i < 6; i++) {
      //   uint32_t old = base[4+i];
      //   base[4+i] = 0xffffffff;
      //   __sync_synchronize();
      //   base[4+i] = old;
      // }

      base[4+0] = (uint32_t) E1000_REG;
      e1000_init((uint32_t *)E1000_REG);
    }
  }
}

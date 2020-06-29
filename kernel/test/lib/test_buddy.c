#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "defs.h"
#include "lib/buddy.h"

extern uint64 bd_allocated;

void buddy_test() {
  if (cpuid() == 0) {

    // ticksleep(10);
    printf("\t[buddy test] start...");
    // page alloc
    for (int n = 0; n < 100; n++) {
      void *pages_4096[MAX_PAGES];
      void *pages_2048[MAX_PAGES << 1];
      void *pages_1024[MAX_PAGES << 2];
      // void *pages_512[MAX_PAGES << 3];
      // void *pages_256[MAX_PAGES << 4];
      // void *pages_128[MAX_PAGES << 5];
      // void *pages_64[MAX_PAGES << 6];

      for (int i = 0; i < MAX_PAGES-1; i++) {
        pages_4096[i] = bd_alloc(PGSIZE);
        if (pages_4096[i] == 0) {
          printf("n: %d, i: %d\n", n, i);
          panic("bd_alloc failed!");
        }
        memset(pages_4096[i], n % 16, PGSIZE);
      }
      for (int i = 0; i < MAX_PAGES-1; i++) {
        for (int j = 0; j < PGSIZE; j++) {
          if ((((char *)pages_4096[i])[j] != n % 16)) {
            panic("\nbuddy test failed...");
          }
        }
      }
      for (int i = 0; i < MAX_PAGES-1; i++) {
        bd_free(pages_4096[i]);
      }

      // 2048
      for (int i = 0; i < (MAX_PAGES-1) * 2; i++) {
        pages_2048[i] = bd_alloc(2048);
        if (pages_2048[i] == 0) {
          printf("n: %d, i: %d\n", n, i);
          panic("bd_alloc failed!");
        }
        memset(pages_2048[i], n % 16, 2048);
      }
      for (int i = 0; i < (MAX_PAGES-1) * 2; i++) {
        for (int j = 0; j < 2048; j++) {
          if ((((char *)pages_2048[i])[j] != n % 16)) {
            panic("\nbuddy test failed...");
          }
        }
      }
      for (int i = 0; i < (MAX_PAGES-1) * 2; i++) {
        bd_free(pages_2048[i]);
      }

      // 1024
      for (int i = 0; i < (MAX_PAGES-1) * 4; i++) {
        pages_1024[i] = bd_alloc(1024);
        if (pages_1024[i] == 0) {
          printf("n: %d, i: %d\n", n, i);
          panic("bd_alloc failed!");
        }
        memset(pages_1024[i], n % 16, 1024);
      }
      for (int i = 0; i < (MAX_PAGES-1) * 4; i++) {
        for (int j = 0; j < 1024; j++) {
          if ((((char *)pages_1024[i])[j] != n % 16)) {
            panic("\nbuddy test failed...");
          }
        }
      }
      for (int i = 0; i < (MAX_PAGES-1) * 4; i++) {
        bd_free(pages_1024[i]);
      }

      // 512
      // for (int i = 0; i < (MAX_PAGES-1) * 8; i++) {
      //   pages_512[i] = bd_alloc(512);
      //   if (pages_512[i] == 0) {
      //     printf("n: %d, i: %d\n", n, i);
      //     panic("bd_alloc failed!");
      //   }
      //   memset(pages_512[i], n % 16, 512);
      // }
      // for (int i = 0; i < (MAX_PAGES-1) * 8; i++) {
      //   for (int j = 0; j < 512; j++) {
      //     if ((((char *)pages_512[i])[j] != n % 16)) {
      //       panic("\nbuddy test failed...");
      //     }
      //   }
      // }
      // for (int i = 0; i < (MAX_PAGES-1) * 8; i++) {
      //   bd_free(pages_512[i]);
      // }

      // 256
      // for (int i = 0; i < (MAX_PAGES-1) * 16; i++) {
      //   pages_256[i] = bd_alloc(256);
      //   if (pages_256[i] == 0) {
      //     printf("n: %d, i: %d\n", n, i);
      //     panic("bd_alloc failed!");
      //   }
      //   memset(pages_256[i], n % 16, 256);
      // }
      // for (int i = 0; i < (MAX_PAGES-1) * 16; i++) {
      //   for (int j = 0; j < 256; j++) {
      //     if ((((char *)pages_256[i])[j] != n % 16)) {
      //       panic("\nbuddy test failed...");
      //     }
      //   }
      // }
      // for (int i = 0; i < (MAX_PAGES-1) * 16; i++) {
      //   bd_free(pages_256[i]);
      // }
    }
    printf("finish! \n");
  }
}
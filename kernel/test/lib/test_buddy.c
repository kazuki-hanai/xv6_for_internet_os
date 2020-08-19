#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "defs.h"
#include "lib/buddy.h"

extern uint64_t bd_allocated;

void buddy_test() {
  printf("\t[buddy test] start...\n");
  // page alloc

  // 64
  for (int n = 0; n < 2; n++) {
    void *pages_64[MAX_PAGES << 7];
    for (int i = 0; i < (MAX_PAGES-1) * 32; i++) {
      printf("alloc: i: %d, max: %d\n", i, (MAX_PAGES-1) * 32);
      pages_64[i] = bd_alloc(64);
      if (pages_64[i] == 0) {
        panic("bd_alloc failed!");
      }
      memset(pages_64[i], n % 16, 64);
    }
    // for (int i = 0; i < (MAX_PAGES-1) * 32; i++) {
    //   printf("check: i: %d\n", i);
    //   for (int j = 0; j < 64; j++) {
    //     if ((((char *)pages_64[i])[j] != n % 16)) {
    //       panic("\nbuddy test failed...");
    //     }
    //   }
    // }
    for (int i = 0; i < (MAX_PAGES-1) * 32; i++) {
      bd_free(pages_64[i]);
    }
  }
  printf("\t\t64...done!\n");

  // 4096
  for (int n = 0; n < 100; n++) {
    void *pages_4096[MAX_PAGES];
    for (int i = 0; i < MAX_PAGES-1; i++) {
      pages_4096[i] = bd_alloc(PGSIZE);
      if (pages_4096[i] == 0) {
        printf("n: %d, i: %d\n", n, i);
        panic("bd_alloc failed!");
      }
      memset(pages_4096[i], n % 16, PGSIZE);
    }
    printf("n: %d\n", n);
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
  }
  printf("\t\t4096...done!\n");

  // // 2048
  // for (int n = 0; n < 100; n++) {
  //   void *pages_2048[MAX_PAGES << 1];
  //   for (int i = 0; i < (MAX_PAGES-1) * 2; i++) {
  //     pages_2048[i] = bd_alloc(2048);
  //     if (pages_2048[i] == 0) {
  //       printf("n: %d, i: %d\n", n, i);
  //       panic("bd_alloc failed!");
  //     }
  //     memset(pages_2048[i], n % 16, 2048);
  //   }
  //   for (int i = 0; i < (MAX_PAGES-1) * 2; i++) {
  //     for (int j = 0; j < 2048; j++) {
  //       if ((((char *)pages_2048[i])[j] != n % 16)) {
  //         panic("\nbuddy test failed...");
  //       }
  //     }
  //   }
  //   for (int i = 0; i < (MAX_PAGES-1) * 2; i++) {
  //     bd_free(pages_2048[i]);
  //   }
  // }
  // printf("\t\t2048...done!\n");

  // // 1024
  // for (int n = 0; n < 30; n++) {
  //   void *pages_1024[MAX_PAGES << 2];
  //   for (int i = 0; i < (MAX_PAGES-1) * 4; i++) {
  //     pages_1024[i] = bd_alloc(1024);
  //     if (pages_1024[i] == 0) {
  //       printf("n: %d, i: %d\n", n, i);
  //       panic("bd_alloc failed!");
  //     }
  //     memset(pages_1024[i], n % 16, 1024);
  //   }
  //   for (int i = 0; i < (MAX_PAGES-1) * 4; i++) {
  //     for (int j = 0; j < 1024; j++) {
  //       if ((((char *)pages_1024[i])[j] != n % 16)) {
  //         panic("\nbuddy test failed...");
  //       }
  //     }
  //   }
  //   for (int i = 0; i < (MAX_PAGES-1) * 4; i++) {
  //     bd_free(pages_1024[i]);
  //   }
  // }
  // printf("\t\t1024...done!\n");

  // // 512
  // for (int n = 0; n < 10; n++) {
  //   void *pages_512[MAX_PAGES << 3];
  //   for (int i = 0; i < (MAX_PAGES-1) * 8; i++) {
  //     pages_512[i] = bd_alloc(512);
  //     if (pages_512[i] == 0) {
  //       printf("n: %d, i: %d\n", n, i);
  //       panic("bd_alloc failed!");
  //     }
  //     memset(pages_512[i], n % 16, 512);
  //   }
  //   for (int i = 0; i < (MAX_PAGES-1) * 8; i++) {
  //     for (int j = 0; j < 512; j++) {
  //       if ((((char *)pages_512[i])[j] != n % 16)) {
  //         panic("\nbuddy test failed...");
  //       }
  //     }
  //   }
  //   for (int i = 0; i < (MAX_PAGES-1) * 8; i++) {
  //     bd_free(pages_512[i]);
  //   }
  // }
  // printf("\t\t512...done!\n");

  // // 256
  // for (int n = 0; n < 10; n++) {
  //   void *pages_256[MAX_PAGES << 4];
  //   for (int i = 0; i < (MAX_PAGES-1) * 16; i++) {
  //     pages_256[i] = bd_alloc(256);
  //     if (pages_256[i] == 0) {
  //       printf("n: %d, i: %d\n", n, i);
  //       panic("bd_alloc failed!");
  //     }
  //     memset(pages_256[i], n % 16, 256);
  //   }
  //   for (int i = 0; i < (MAX_PAGES-1) * 16; i++) {
  //     for (int j = 0; j < 256; j++) {
  //       if ((((char *)pages_256[i])[j] != n % 16)) {
  //         panic("\nbuddy test failed...");
  //       }
  //     }
  //   }
  //   for (int i = 0; i < (MAX_PAGES-1) * 16; i++) {
  //     bd_free(pages_256[i]);
  //   }
  // }
  // printf("\t\t256...done!\n");

  // // 128
  // for (int n = 0; n < 2; n++) {
  //   void *pages_128[MAX_PAGES << 6];
  //   for (int i = 0; i < (MAX_PAGES-1) * 32; i++) {
  //     pages_128[i] = bd_alloc(128);
  //     if (pages_128[i] == 0) {
  //       printf("n: %d, i: %d\n", n, i);
  //       panic("bd_alloc failed!");
  //     }
  //     memset(pages_128[i], n % 16, 128);
  //   }
  //   for (int i = 0; i < (MAX_PAGES-1) * 32; i++) {
  //     for (int j = 0; j < 128; j++) {
  //       if ((((char *)pages_128[i])[j] != n % 16)) {
  //         panic("\nbuddy test failed...");
  //       }
  //     }
  //   }
  //   for (int i = 0; i < (MAX_PAGES-1) * 32; i++) {
  //     bd_free(pages_128[i]);
  //   }
  // }
  // printf("\t\t128...done!\n");


  printf("finish! \n");
}

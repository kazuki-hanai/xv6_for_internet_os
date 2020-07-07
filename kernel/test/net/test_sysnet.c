#include "sys/sysnet.h"
#include "defs.h"
#include "test/test.h"

void sport_test();

void sysnet_test() {
  printf("\t[sysnet test] start...\n");
  sport_test();
  printf("\t[sysnet test] done!\n");
}

// get_new_sport()
// release_sport()
void sport_test() {
  uint16 sport[128];

  printf("\t\t[sport test] start...\n");
  for (int k = 0; k < 2; k++) {
    for (int i = 0; i < 128; i++) {
      sport[i] = get_new_sport(); 
      if (sport[i] != START_OF_SPORT + i ) {
        panic("sport does not match");
      }
    }

    release_sport(sport[0]);
    sport[0] = 0;
    sport[0] = get_new_sport();
    if (sport[0] != START_OF_SPORT) {
      panic("released sport does not match");
    }

    for (int i = 0; i < 128; i++) {
      release_sport(sport[i]);
      sport[i] = 0;
    }
  }
  printf("\t\t[sport test] done!\n");
}
#pragma once

#include "types.h"
#include "net/sock_cb.h"

#define START_OF_SPORT 20000
#define SPORT_NUM 100
#define MAX_SPORT (START_OF_SPORT + 100 * 8);

#define SPORT_IS_USED(x) (sport_table[x/8] >> (x % 8) & 0x01)

uint16 get_new_sport();
void release_sport(uint16);

#pragma once

#include "types.h"

#define SOCK_PROTO_TCP 1
#define SOCK_PROTO_UDP 2

#define SOCK_LISTEN 1
#define SOCK_CONNECT 2

#define START_OF_SPORT 20000
#define SPORT_NUM 100;
#define MAX_SPORT (START_OF_SPORT + 100 * 8);

uint8 sport_table[SPORT_NUM];
#define SPORT_IS_USED(x) (sport_table[x/8] >> (x % 8) & 0x01)

uint16 get_new_sport();
void release_sport(uint16);

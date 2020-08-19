#pragma once

#include "types.h"
#include "file.h"
#include "net/sock_cb.h"

#define START_OF_SPORT 20000
#define SPORT_NUM 3000
#define MAX_SPORT SPORT_NUM * 8

#define SPORT_IS_USED(x) (sport_table[x/8] >> (x % 8) & 0x01)

void socket_init();
struct sock_cb* sockalloc(int socktype);
uint64_t socklisten(struct sock_cb *, uint16_t);
uint64_t sockconnect(struct sock_cb *, uint32_t, uint16_t);
int socksend(struct sock_cb*, uint64_t, int, int);
int sockrecv(struct sock_cb*, uint64_t, int, int);
void sockclose(struct sock_cb*);

uint16_t get_new_sport();
uint16_t get_specified_sport(uint16_t);
void release_sport(uint16_t);

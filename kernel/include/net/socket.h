#pragma once

#include "types.h"
#include "file.h"
#include "net/sock_cb.h"

#define START_OF_SPORT 20000
#define SPORT_NUM 16
#define MAX_SPORT (START_OF_SPORT + SPORT_NUM * 8)

#define SPORT_IS_USED(x) (sport_table[x/8] >> (x % 8) & 0x01)

void socket_init();
struct sock_cb* sockalloc(int socktype);
uint64 socklisten(struct sock_cb *, uint16);
uint64 sockconnect(struct sock_cb *, uint32, uint16);
int socksend(struct file *, uint64, int, int);
int sockrecv(struct file *, uint64, int, int);
void sockclose(struct file *);

uint16 get_new_sport();
uint16 get_specified_sport(uint16);
void release_sport(uint16);
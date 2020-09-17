#pragma once

#include "types.h"
#include "file.h"
#include "net/sock_cb.h"

#define START_OF_SPORT 20000
#define SPORT_NUM 65535
#define SPORT_ELEM SPORT_NUM / 8

#define SPORT_IS_USED(x) (sport_table[x/8] >> (x % 8) & 0x01)

void             socket_init();
struct file*     sockalloc(int socktype);
struct file*     sockcopy(struct file* f);
uint64_t         sockaccept(struct sock_cb *, uint32_t*, uint16_t*);
uint64_t         socklisten(struct sock_cb *, uint16_t);
uint64_t         sockconnect(struct sock_cb *, uint32_t, uint16_t);
int              socksend(struct sock_cb*, uint64_t, int, int);
int              sockrecv(struct sock_cb*, uint64_t, int, int);
void             sockclose(struct sock_cb*);

uint16_t         get_new_sport();
uint16_t         get_specified_sport(uint16_t);
void             release_sport(uint16_t);

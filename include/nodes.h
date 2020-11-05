
#include "spinlock.h"
#include <stdint.h>

#define NODE_HASH_NUM 64
#define ALIAS_LENGTH 16

struct node {
	uint64_t nid;
	char alias[ALIAS_LENGTH];
	struct node* next;
};

struct node_map {
	struct spinlock lock;
	int             client_num;
        struct node*    n[NODE_HASH_NUM];
};

void node_init();
int node_add(uint64_t nid);
int node_remove(uint64_t nid);

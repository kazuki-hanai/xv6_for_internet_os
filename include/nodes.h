
#include "spinlock.h"
#include <stdint.h>

#define NODE_HASH_NUM 64

struct node {
	uint64_t nid;
	struct node* next;
};

struct node_map {
	struct spinlock lock;
        struct node* n[NODE_HASH_NUM];
};


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
	int             nodesnum;
        struct node*    n[NODE_HASH_NUM];
};

void node_init();
struct node** node_getnodes(int* num);
int node_getnum();
int node_add(uint64_t nid);
int node_exist(uint64_t nid);
int node_remove(uint64_t nid);

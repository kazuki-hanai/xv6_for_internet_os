#include "types.h"
#include "defs.h"
#include "nodes.h"

struct node_map nodemap;

uint64_t nodemap_hash(uint64_t nid) {
        return nid % NODE_HASH_NUM;
}

static int nodemap_add(uint64_t nid) {
        uint64_t hash = nodemap_hash(nid);

        struct node* n;
        n = (struct node*) ufkalloc(sizeof(struct node));
        n->nid = nid;
        n->next = 0;

        acquire(&nodemap.lock);
        if (nodemap.n[hash] == 0) {
                nodemap.n[hash] = n;
        } else {
                struct node* now = nodemap.n[hash];
                while (now->next != 0) {
                        now = now->next;
                }
                now->next = n;
        }
        nodemap.client_num += 1;
        release(&nodemap.lock);

        return 0;
}

static int nodemap_exist(uint64_t nid) {
	uint64_t hash = nodemap_hash(nid);

	acquire(&nodemap.lock);
	struct node* n = nodemap.n[hash];
	while (n->next != 0) {
		if (n->nid == nid)
			break;
		n = n->next;
	}
	release(&nodemap.lock);

	return n->nid == nid;
}

static int nodemap_remove(uint64_t nid) {
        uint64_t hash = nodemap_hash(nid);

        acquire(&nodemap.lock);
        if (nodemap.n[hash] == 0) {
                return -1;
        } else {
                struct node* now = nodemap.n[hash];
                struct node* prev = 0;
                while (now->next != 0) {
                        if (now->nid == nid) {
                                if (prev != 0)
                                        prev->next = now->next;
				ufkfree(now);
                                return 0;
                        } 
                        prev = now;
                        now = now->next;
                }
                return -1;
        }
        nodemap.client_num -= 1;
        release(&nodemap.lock);
}

void node_init() {
        memset(&nodemap, 0, sizeof(nodemap));
        initlock(&nodemap.lock, "nodelist lock");
}

int node_add(uint64_t nid) {
        return nodemap_add(nid);
}

int node_exist(uint64_t nid) {
	return nodemap_exist(nid);
}

int node_remove(uint64_t nid) {
        return nodemap_remove(nid);
}

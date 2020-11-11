#include "types.h"
#include "defs.h"
#include "nodes.h"

uint64_t sys_getnodes(void) {
	uint64_t addr;
	if (argaddr(0, &addr) < 0) {
		return -1;
	}
	return 0;
}

uint64_t sys_existnode(void) {
	uint64_t nid;
	if (arguint64(0, &nid) < 0) {
		return -1;
	}

	if (node_add(nid) < 0) {
		return -1;
	}

	return 0;
}

uint64_t sys_addnode(void) {
	uint64_t nid;
	if (arguint64(0, &nid) < 0) {
		return -1;
	}

	if (node_add(nid) < 0) {
		return -1;
	}

	return 0;
}

uint64_t sys_removenode(void) {
	uint64_t nid;
	if (arguint64(0, &nid) < 0) {
		return -1;
	}

	if (node_remove(nid) < 0) {
		return -1;
	}

	return 0;
}

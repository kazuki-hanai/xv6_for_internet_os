#include "user.h"
#include "stat.h"
#include "../p9.h"

uint8_t to_qid_type(uint16_t t) {
	uint8_t res = 0;
	if (t & T_DIR)
		res |= P9_ODIR;
	return res;
}

void* p9malloc(int size) {
	void* a = malloc(size);
	if (a == 0) {
		printf("cannot malloc\n");
		exit(1);
	}
	return a;
}

int p9open(char* path, int mode) {
	return open(path, mode);
}

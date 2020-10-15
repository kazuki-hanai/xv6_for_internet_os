#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char **argv)
{
	int n = atoi(argv[1]);
	int start = uptime();
	const int NUM_LOOP = 1000000000;
	const int NUM_SYSCALL = n;
	uint64_t res = 0;
	for (int i = 0; i < NUM_SYSCALL; i++) {
		res += calc(NUM_SYSCALL);
	}

	int end = uptime();

	printf("num_syscall: %d, num_loop: %d, res: %d, time: %d\n", NUM_SYSCALL, NUM_LOOP/NUM_SYSCALL, res, end-start);

	exit(0);
}

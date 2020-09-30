#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char **argv)
{
		int n1, n2;
		if(argc < 2) {
				fprintf(2, "usage: uptime ticks\n");
				exit(1);
		}
		n1 = uptime();
		sleep(atoi(argv[1]));
		n2 = uptime();
		fprintf(2, "uptime: %d ticks\n", n2-n1);
		exit(0);
}

#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char **argv)
{
	int p[2];
	if (pipe(p) == -1) {
		printf("error occured!\n");
		exit(1);
	}
	char buf[128];
	if (fork() == 0) {
		write(p[1], "test", 4);
		setnonblock(p[0], 1);
		while(read(p[0], buf, 6) == 0)
			;
		printf("%s\n", buf);
	} else {
		read(p[0], buf, 4);
		printf("%s\n", buf);
		sleep(10);
		write(p[1], "hello!", 6);
		int cpid = wait(0);
		printf("cpid: %d\n", cpid);
	}
	exit(0);
}

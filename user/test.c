#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char **argv)
{
	int p[2];
	
	if (pipe(p) < 0) {
		printf("cannot pipe\n");
		exit(1);
	}
	int pid = fork();
	if (pid < 0) {
		printf("cannot fork\n");
		exit(1);
	}
	if(pid > 0) {
		// close(p[1]);
		printf("root: wait()\n");
		int status;
		if (wait(&status) >= 0) {
			printf("child exited: %d\n", status);
		} else {
			printf("cannot wait\n");
			exit(1);
		}
	} else {
		sleep(10);
		// printf("child\n");
		// close(p[0]);
		// printf("child2\n");
		// int a;
		// if(read(p[0][0], &a, sizeof(a)) > 0) {
		// 	printf("%d", a);
		// } else {
		// 	printf("-1\n");
		// }
	}
	exit(0);
}

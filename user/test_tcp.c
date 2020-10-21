#include "types.h"
#include "stat.h"
#include "net/sock_cb.h"
#include "user.h"

int main(int argc, char **argv)
{
	int sock = socket(SOCK_TCP);
	listen(sock, 2000);
	if (setnonblock(sock, 1) < 0) {
		printf("cnanot setnonblock\n");
		exit(1);
	}

	int pid = -1;
	int cfd = -1;
	uint32_t raddr;
	uint16_t dport;
	while(1) {
		// accept part
		cfd = accept(sock, &raddr, &dport);
		if (cfd != -1) {
			printf("accept: %x:%d\n", raddr, dport);
			pid = fork();
			if (pid == 0)
				break;
		}
	}
	while(1) {
		char buf[128];
		int rsize = read(cfd, buf, 128);
		write(cfd, buf, rsize);
	}
	exit(0);
}

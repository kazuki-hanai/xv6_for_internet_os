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
	int cfds[16];
	int p[16][2];
	int cnum = 0;
	while(1) {
		// accept part
		cfd = accept(sock, &raddr, &dport);
		if (cfd != -1) {
			printf("accept: %d, %x:%d\n", cfd, raddr, dport);
			cfds[cnum] = cfd;

			pipe(p[cnum]);
			if (setnonblock(p[cnum][0], 1) < 0) {
				printf("cnanot setnonblock\n");
				exit(1);
			}

			pid = fork();
			if (pid == 0)
				break;
			cnum += 1;
		}
		for (int i = 0; i < cnum; i++) {
			char buf[128];
			int rsize = read(p[i][0], buf, 128);
			if (rsize == -1)
				continue;
			for (int i = 0; i < cnum; i++) {
				int wsize = write(cfds[i], buf, rsize);
				if (wsize == -1) {
					printf("[Error] Client disconnected\n");
					continue;
				}
			}
		}
		
	}
	while(1) {
		char buf[128];
		int rsize = read(cfd, buf, 128);
		if (rsize == -1) {
			printf("Client disconnected\n");
			break;
		}
		if (rsize == 0) {
			continue;
		}
		write(p[cnum][1], buf, rsize);
	}
	exit(0);
}

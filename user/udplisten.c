#include "types.h"
#include "stat.h"
#include "net/sock_cb.h"
#include "user.h"

uint32_t get_ip(char *ip) {
	int len = strlen(ip);
	int b = 0;
	uint32_t res = 0;
	for (int i = 0; i < len; i++) {
		if (ip[i] == '.') {
			res <<= 8;
			res += b;
			b = 0;
		} else {
			if('0' <= ip[i] && ip[i] <= '9')
				b = b*10 + ip[i] - '0'; 
			else
				return 0;
		}
	}
	res <<= 8;
	res += b;
	return res;
}

int
main(int argc, char **argv)
{
	if (argc < 2) {
		printf("usage: %s port\n", argv[0]);
		exit(1);
	}
	// uint32_t raddr = get_ip(argv[1]);
	uint16_t sport = atoi(argv[1]);
	// uint16_t dport = atoi(argv[2]);
	int sock;

	sock = socket(SOCK_UDP);
	if (listen(sock, sport) < 0) {
		printf("listen failed!\n");
		close(sock);
		exit(1);
	}

	while(1) {
		char rbuf[256];
		char wbuf[256];

		read(sock, rbuf, 256);
		printf("you: %s\n", rbuf);

		printf("me: ");
		int wsize = read(1, wbuf, 256);
		write(sock, wbuf, wsize);
	}

	close(sock);
	exit(0);
}


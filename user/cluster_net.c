#include "types.h"
#include "user.h"
#include "net/sock_cb.h"

#define MAX_MSG 1024

const uint8_t LIST_REQ = 1;
const uint8_t LIST_REP = 2;

struct __attribute__((packed)) clster_hdr {
	uint8_t  ctype;
	uint64_t nid;
};

int read_msg(int csock, struct clster_hdr* hdr) {
	char buf[MAX_MSG];
	int rsize = -1;
	if ((rsize = read(csock, buf, MAX_MSG)) < 0) {
		return -1;
	}
	hdr = (struct clster_hdr*) buf;

	return 0;
}

int send_msg(int csock, struct clster_hdr* hdr) {

}

int main(int argc, char **argv) {
	int sock = socket(SOCK_TCP);
	listen(sock, 4000);
	uint32_t raddr;
	uint16_t dport;
	int csock;
	
	while(1) {
		csock = accept(sock, &raddr, &dport);
		if (fork() != 0) {
			break;
		} 
	}

	// communicate with messages
	while(1) {
		struct clster_hdr hdr;
		if (read_msg(csock, &hdr) < 0) {

		}

		if (send_msg(csock, &hdr) < 0) {
			exit(1);
		}
	}

	exit(0);
	return 0;
}

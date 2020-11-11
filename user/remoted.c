#include "types.h"
#include "user.h"
#include "net/sock_cb.h"

#define MAX_MSG 1024
#define REMOTED_PORT 2091

const uint8_t LIST_REQ = 1;
const uint8_t LIST_REP = 2;

struct __attribute__((packed)) clster_hdr {
	uint8_t  ctype;
	uint64_t nid;
};

static int read_msg(int csock, struct clster_hdr* hdr) {
	char buf[MAX_MSG];
	int rsize = -1;
	if ((rsize = read(csock, buf, MAX_MSG)) < 0) {
		return -1;
	}
	hdr = (struct clster_hdr*) buf;

	if (!existnode(hdr->nid)) {
		addnode(hdr->nid);
		printf("node added: %x\n", hdr->nid);
	}

	hdr->ctype = LIST_REP;

	return 0;
}

static int send_msg(int csock, struct clster_hdr* hdr) {
	char buf[MAX_MSG];

	memmove(buf, hdr, sizeof(*hdr));

	int wsize = -1;
	if ((wsize = write(csock, buf, MAX_MSG)) < 0) {
		return -1;
	}
	return 0;
}

static void start_remoted() {
	int sock = socket(SOCK_TCP);
	listen(sock, REMOTED_PORT);
	
	uint32_t raddr;
	uint16_t dport;
	int csock;
	
	while(1) {
		csock = accept(sock, &raddr, &dport);
		if (csock > 0 && fork() != 0) {
			printf("accepted: %x:%x\n", raddr, dport);
			break;
		} 
	}

	// communicate with messages
	while(1) {
		struct clster_hdr hdr;
		if (read_msg(csock, &hdr) < 0) {
			exit(1);
		}

		if (send_msg(csock, &hdr) < 0) {
			exit(1);
		}
	}

	exit(0);
}

int main(int argc, char **argv) {
	if (fork() == 0) {
		start_remoted();
	}
	return 0;
}

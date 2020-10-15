#include "user.h"
#include "net/byteorder.h"
#include "net/socket.h"
#include "p9.h"

struct p9_req* p9_allocreq() {
	struct p9_req *req;
	req = p9malloc(sizeof(*req));
	memset(req, 0, sizeof *req);
	return req;
}

void p9_freereq(struct p9_req *req) {
	if (req->ofcall.type == P9_RREAD) {
		free(req->ofcall.data);
	} else if (req->ofcall.type == P9_RSTAT) {
		p9_freestat(req->ofcall.stat);
	} else if (req->ifcall.type == P9_TWRITE) {
		free(req->ifcall.data);
	}
	free(req);
}

int p9_sendreq(struct p9_conn *conn) {
	if (write(conn->sockfd, conn->wbuf, conn->wsize) <= 0) {
		return -1;
	}
	return 0;
}

struct p9_req* p9_recvreq(struct p9_conn *conn) {
	if (conn->rsize == 0) {
		if ((conn->rsize = read(conn->sockfd, conn->_rbuf, P9_MAXMSGLEN)) == -1) {
			return 0;
		}
		int size = GBIT32(conn->_rbuf);
		while(conn->rsize < size) {
			if ((conn->rsize += read(conn->sockfd, conn->_rbuf+conn->rsize, P9_MAXMSGLEN-conn->rsize)) <= 0) {
				return 0;
			}
		}
		conn->rbuf = conn->_rbuf;
	}

	int size = GBIT32(conn->rbuf);

	struct p9_req *req;
	if ((req = parsefcall(conn->rbuf, size)) == 0) {
		return 0;
	}

	conn->rsize -=  size;
	conn->rbuf += size;

	return req;
}

//
// network system calls.
//
#include "types.h"
#include "defs.h"
#include "file.h"
#include "net/socket.h"
#include "proc.h"

uint64_t
sys_socket(void)
{
	struct file *f;
	int socktype;

	if(
		argint(0, (int *)&socktype) < 0
	)
		return -1;

	if ((f = sockalloc(socktype)) == 0)
		goto bad;

	int fd;
	if((fd = fdalloc(f)) < 0)
		return -1;

	return fd;

bad:
	if (f)
		fileclose(f);
	return -1;
}

uint64_t sys_socklisten() {
	int fd;
	struct file *f;
	struct sock_cb *scb;
	uint16_t sport;

	if (argint(1, (int *)&sport) < 0 || argfd(0, &fd, &f) < 0 ) {
		return -1;
	}

	scb = f->scb;
	// file doesn*t equal socket or socket close
	if (f->type !=  FD_SOCK || scb == 0) {
		return -1;
	}
	
	return socklisten(scb, sport);
}

uint64_t sys_sockaccept() {
	int fd;
	struct file *f;
	struct sock_cb *scb;
	struct proc *p = myproc();
	uint64_t raddrp;
	uint64_t dportp;
	uint32_t raddr;
	uint16_t dport;

	if (argaddr(2, (uint64_t *)&dportp) < 0 || argaddr(1, (uint64_t *)&raddrp) < 0 || argfd(0, &fd, &f) < 0) {
		return -1;
	}

	scb = f->scb;
	if (f->type !=  FD_SOCK || scb == 0) {
		return -1;
	}
	int res = sockaccept(scb, &raddr, &dport);

	copyout(p->pagetable, raddrp, (char*)&raddr, sizeof(uint32_t));
	copyout(p->pagetable, dportp, (char*)&dport, sizeof(uint16_t));

	return res;
}


uint64_t sys_sockconnect() {
	struct file *f;
	struct sock_cb *scb;
	uint32_t raddr;
	uint16_t dport;

	if (argint(2, (int *)&dport) < 0 || argint(1, (int *)&raddr) < 0 || argfd(0, 0, &f) < 0) {
		return -1;
	}

	scb = f->scb;
	// file doesn*t equal socket or socket close
	if (f->type !=  FD_SOCK || scb == 0) {
		return -1;
	}

	return sockconnect(scb, raddr, dport); 
}

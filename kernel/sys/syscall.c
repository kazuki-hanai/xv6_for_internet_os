#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "sys/syscall.h"
#include "file.h"
#include "defs.h"

// Fetch the uint64_t at addr from the current process.
int
fetchaddr(uint64_t addr, uint64_t *ip)
{
	struct proc *p = myproc();
	if(addr >= p->sz || addr+sizeof(uint64_t) > p->sz)
		return -1;
	if(copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0)
		return -1;
	return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.
int
fetchstr(uint64_t addr, char *buf, int max)
{
	struct proc *p = myproc();
	int err = copyinstr(p->pagetable, buf, addr, max);
	if(err < 0)
		return err;
	return strlen(buf);
}

static uint64_t
argraw(int n)
{
	struct proc *p = myproc();
	switch (n) {
	case 0:
		return p->tf->a0;
	case 1:
		return p->tf->a1;
	case 2:
		return p->tf->a2;
	case 3:
		return p->tf->a3;
	case 4:
		return p->tf->a4;
	case 5:
		return p->tf->a5;
	}
	panic("argraw");
	return -1;
}

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
	*ip = argraw(n);
	return 0;
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
int
argaddr(int n, uint64_t *ip)
{
	*ip = argraw(n);
	return 0;
}

// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (including nul), -1 if error.
int
argstr(int n, char *buf, int max)
{
	uint64_t addr;
	if(argaddr(n, &addr) < 0)
		return -1;
	return fetchstr(addr, buf, max);
}

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
int
argfd(int n, int *pfd, struct file **pf)
{
	int fd;
	struct file *f;
	
	if(argint(n, &fd) < 0)
		return -1;
	if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
		return -1;
	if(pfd)
		*pfd = fd;
	if(pf)
		*pf = f;
	return 0;
}

extern uint64_t sys_chdir(void);
extern uint64_t sys_close(void);
extern uint64_t sys_dup(void);
extern uint64_t sys_exec(void);
extern uint64_t sys_exit(void);
extern uint64_t sys_fork(void);
extern uint64_t sys_fstat(void);
extern uint64_t sys_getpid(void);
extern uint64_t sys_kill(void);
extern uint64_t sys_link(void);
extern uint64_t sys_mkdir(void);
extern uint64_t sys_mknod(void);
extern uint64_t sys_open(void);
extern uint64_t sys_pipe(void);
extern uint64_t sys_read(void);
extern uint64_t sys_sbrk(void);
extern uint64_t sys_sleep(void);
extern uint64_t sys_unlink(void);
extern uint64_t sys_wait(void);
extern uint64_t sys_write(void);
extern uint64_t sys_uptime(void);
// sysnet
extern uint64_t sys_socket(void);
extern uint64_t sys_sockconnect(void);
extern uint64_t sys_socklisten(void);
extern uint64_t sys_sockaccept(void);
// sysremote
extern uint64_t sys_getnodes(void);
extern uint64_t sys_addnode(void);
extern uint64_t sys_removenode(void);

uint64_t sys_calc() {
	int num;
	uint64_t res = 0;

	if(argint(0, &num) < 0)
		return -1;

	const int NUM_LOOP = 1000000000 / num;
 	for (int i = 0; i < NUM_LOOP; i++) {
 		res += 1;
 	}

	return res;
}

static uint64_t (*syscalls[])(void) = {
[SYS_fork]        sys_fork,
[SYS_exit]        sys_exit,
[SYS_wait]        sys_wait,
[SYS_pipe]        sys_pipe,
[SYS_read]        sys_read,
[SYS_kill]        sys_kill,
[SYS_exec]        sys_exec,
[SYS_fstat]       sys_fstat,
[SYS_chdir]       sys_chdir,
[SYS_dup]         sys_dup,
[SYS_getpid]      sys_getpid,
[SYS_sbrk]        sys_sbrk,
[SYS_sleep]       sys_sleep,
[SYS_uptime]      sys_uptime,
[SYS_open]        sys_open,
[SYS_write]       sys_write,
[SYS_mknod]       sys_mknod,
[SYS_unlink]      sys_unlink,
[SYS_link]        sys_link,
[SYS_mkdir]       sys_mkdir,
[SYS_close]       sys_close,
[SYS_socket]      sys_socket,
[SYS_connect]     sys_sockconnect,
[SYS_listen]      sys_socklisten,
[SYS_accept]      sys_sockaccept,
[SYS_calc]        sys_calc,
[SYS_getnodes]    sys_getnodes,
[SYS_addnode]     sys_addnode,
[SYS_removenode]  sys_removenode,
};

void
syscall(void)
{
	int num;
	struct proc *p = myproc();

	num = p->tf->a7;
	if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
		p->tf->a0 = syscalls[num]();
	} else {
		printf("%d %s: unknown sys call %d\n",
						p->pid, p->name, num);
		p->tf->a0 = -1;
	}
}

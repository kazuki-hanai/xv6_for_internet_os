//
// Support functions for system calls that involve file descriptors.
//

#include "types.h"
#include "arch/riscv.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"
#include "stat.h"
#include "proc.h"
#include "net/socket.h"

struct devsw devsw[NDEV];

struct {
	struct spinlock lock;
	struct file *file;
} ftable;

void
fileinit(void)
{
	initlock(&ftable.lock, "ftable");
	ftable.file = 0;
}

// Allocate a file structure.
struct file*
filealloc(void)
{
	struct file *p, *f;

	acquire(&ftable.lock);
	for(p = 0, f = ftable.file; f != 0; p = f, f = f->next)
		;
	// alloc
	f = ufkalloc(sizeof(struct file));
	f->next = 0;
	f->ref = 0;
	if (p != 0) {
		f->prev = p;
		p->next = f;
	} else {
		f->prev = 0;
	}
	if(f->ref == 0){
		f->ref = 1;
		release(&ftable.lock);
		return f;
	}
	release(&ftable.lock);
	return 0;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
	acquire(&ftable.lock);
	if(f->ref < 1)
		panic("filedup");
	f->ref++;
	release(&ftable.lock);
	return f;
}

int filefree(struct file *f)
{
	acquire(&ftable.lock);
	if(f->ref < 1)
		panic("fileclose");
	if(--f->ref > 0){
		release(&ftable.lock);
		return -1;
	}
	f->ref = 0;
	f->type = FD_NONE;

	if (f->prev == 0) {
		ftable.file = f->next;
		if (f->next != 0)
			f->next->prev = 0;
	} else {
		f->prev->next = f->next;
		f->next->prev = f->prev;
	}
	ufkfree((void*)f);
	release(&ftable.lock);
	return 0;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
	struct file ff;
	ff = *f;

	if (filefree(f) < 0) {
		return;
	}

	if(ff.type == FD_PIPE){
		pipeclose(ff.pipe, ff.writable);
	} else if(ff.type == FD_INODE || ff.type == FD_DEVICE){
		begin_op();
		iput(ff.ip);
		end_op();
	} else if (ff.type == FD_SOCK) {
		ff.scb->f = 0;
		sockclose(ff.scb);
	}
}

// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int
filestat(struct file *f, uint64_t addr)
{
	struct proc *p = myproc();
	struct stat st;
	
	if(f->type == FD_INODE || f->type == FD_DEVICE){
		ilock(f->ip);
		stati(f->ip, &st);
		iunlock(f->ip);
		if(copyout(p->pagetable, addr, (char *)&st, sizeof(st)) < 0)
			return -1;
		return 0;
	}
	return -1;
}

// Read from file f.
// addr is a user virtual address.
int
fileread(struct file *f, uint64_t addr, int n)
{
	int r = 0;

	if(f->readable == 0)
		return -1;

	if(f->type == FD_PIPE){
		r = piperead(f->pipe, addr, n, f->nonblockable);
	} else if(f->type == FD_DEVICE){
		if(f->major < 0 || f->major >= NDEV || !devsw[f->major].read)
			return -1;
		r = devsw[f->major].read(1, addr, n);
	} else if(f->type == FD_INODE){
		ilock(f->ip);
		if((r = readi(f->ip, 1, addr, f->off, n)) > 0)
			f->off += r;
		iunlock(f->ip);
	} else if(f->type == FD_SOCK) {
		r = sockrecv(f->scb, addr, n, 1, f->nonblockable);
	} else {
		panic("fileread");
	}

	return r;
}

// Write to file f.
// addr is a user virtual address.
int
filewrite(struct file *f, uint64_t addr, int n)
{
	int r, ret = 0;

	if(f->writable == 0)
		return -1;

	if(f->type == FD_PIPE){
		ret = pipewrite(f->pipe, addr, n);
	} else if(f->type == FD_DEVICE){
		if(f->major < 0 || f->major >= NDEV || !devsw[f->major].write)
			return -1;
		ret = devsw[f->major].write(1, addr, n);
	} else if(f->type == FD_INODE){
		// write a few blocks at a time to avoid exceeding
		// the maximum log transaction size, including
		// i-node, indirect block, allocation blocks,
		// and 2 blocks of slop for non-aligned writes.
		// this really belongs lower down, since writei()
		// might be writing a device like the console.
		int max = ((MAXOPBLOCKS-1-1-2) / 2) * BSIZE;
		int i = 0;
		while(i < n){
			int n1 = n - i;
			if(n1 > max)
				n1 = max;

			begin_op();
			ilock(f->ip);
			if ((r = writei(f->ip, 1, addr + i, f->off, n1)) > 0)
				f->off += r;
			iunlock(f->ip);
			end_op();

			if(r < 0)
				break;
			if(r != n1)
				panic("short filewrite");
			i += r;
		}
		ret = (i == n ? n : -1);
	} else if (f->type == FD_SOCK) {
		ret = socksend(f->scb, addr, n, 1);
	} else {
		panic("filewrite");
	}

	return ret;
}


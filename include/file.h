#pragma once

#include "sleeplock.h"
#include "fs.h"
#include "net/sock_cb.h"

struct file {
	enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE, FD_SOCK } type;
	int ref; // reference count
	char readable:1;
	char writable:1;
	char nonblockable:1;
	struct pipe *pipe; // FD_PIPE
	struct inode *ip;  // FD_INODE and FD_DEVICE
	struct sock_cb *scb; // FD_SOCK
	uint32_t off;          // FD_INODE
	short major;       // FD_DEVICE
	struct file *prev;
	struct file *next;
};

#define major(dev)  ((dev) >> 16 & 0xFFFF)
#define minor(dev)  ((dev) & 0xFFFF)
#define	mkdev(m,n)  ((uint32_t,((m)<<16| (n)))

// in-memory copy of an inode
struct inode {
	uint32_t dev;           // Device number
	uint32_t inum;          // Inode number
	int ref;            // Reference count
	struct sleeplock lock; // protects everything below here
	int valid;          // inode has been read from disk?

	short type;         // copy of disk inode
	short major;
	short minor;
	short nlink;
	uint32_t size;
	uint32_t addrs[NDIRECT+1];
};

// map major device number to device functions.
struct devsw {
	int (*read)(int, uint64_t, int);
	int (*write)(int, uint64_t, int);
};

extern struct devsw devsw[];

#define CONSOLE 1

struct file*    filealloc(void);
int             filefree(struct file*);
void            fileclose(struct file*);
struct file*    filedup(struct file*);
void            fileinit(void);
int             fileread(struct file*, uint64_t, int n);
int             filestat(struct file*, uint64_t addr);
int             filewrite(struct file*, uint64_t, int n);

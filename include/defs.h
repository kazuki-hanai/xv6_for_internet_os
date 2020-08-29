#pragma once

#include "types.h"
#include "arch/riscv.h"

struct buf;
struct context;
struct file;
struct inode;
struct pipe;
struct proc;
struct spinlock;
struct sleeplock;
struct stat;
struct superblock;
struct mbuf;

// bio.c
void            binit(void);
struct buf*     bread(uint32_t, uint32_t);
void            brelse(struct buf*);
void            bwrite(struct buf*);
void            bpin(struct buf*);
void            bunpin(struct buf*);

// console.c
void            consoleinit(void);
void            consoleintr(int);
void            consputc(int);

// exec.c
int             exec(char*, char**);

// fs.c
void            fsinit(int);
int             dirlink(struct inode*, char*, uint32_t);
struct inode*   dirlookup(struct inode*, char*, uint32_t*);
struct inode*   ialloc(uint32_t, short);
struct inode*   idup(struct inode*);
void            iinit();
void            ilock(struct inode*);
void            iput(struct inode*);
void            iunlock(struct inode*);
void            iunlockput(struct inode*);
void            iupdate(struct inode*);
int             namecmp(const char*, const char*);
struct inode*   namei(char*);
struct inode*   nameiparent(char*, char*);
int             readi(struct inode*, int, uint64_t, uint32_t, uint32_t);
void            stati(struct inode*, struct stat*);
int             writei(struct inode*, int, uint64_t, uint32_t, uint32_t);

// ramdisk.c
void            ramdiskinit(void);
void            ramdiskintr(void);
void            ramdiskrw(struct buf*);

// kalloc.c
void*           kalloc(void);
void            kfree(void *);
void            kinit();

// buddy.c
void*           bd_alloc(int);
void            bd_free(void *plist);
void            bd_init();

// log.c
void            initlog(int, struct superblock*);
void            log_write(struct buf*);
void            begin_op();
void            end_op();

// pipe.c
int             pipealloc(struct file**, struct file**);
void            pipeclose(struct pipe*, int);
int             piperead(struct pipe*, uint64_t, int);
int             pipewrite(struct pipe*, uint64_t, int);

// printf.c
void            printf(char*, ...);
void            panic(char*) __attribute__((noreturn));
void            printfinit(void);

// proc.c
int             cpuid(void);
void            exit(int);
int             fork(void);
int             growproc(int);
pagetable_t     proc_pagetable(struct proc *);
void            proc_freepagetable(pagetable_t, uint64_t);
int             kill(int);
struct cpu*     mycpu(void);
struct cpu*     getmycpu(void);
struct proc*    myproc();
void            procinit(void);
void            scheduler(void) __attribute__((noreturn));
void            sched(void);
void            setproc(struct proc*);
void            sleep(void*, struct spinlock*);
void            userinit(void);
int             wait(uint64_t);
void            wakeup(void*);
void            yield(void);
int             either_copyout(int user_dst, uint64_t dst, void *src, uint64_t len);
int             either_copyin(void *dst, int user_src, uint64_t src, uint64_t len);
void            procdump(void);

// swtch.S
void            swtch(struct context*, struct context*);

// spinlock.c
void            acquire(struct spinlock*);
int             holding(struct spinlock*);
void            initlock(struct spinlock*, char*);
void            release(struct spinlock*);
void            push_off(void);
void            pop_off(void);

// sleeplock.c
void            acquiresleep(struct sleeplock*);
void            releasesleep(struct sleeplock*);
int             holdingsleep(struct sleeplock*);
void            initsleeplock(struct sleeplock*, char*);
void            ticksleep(int);

// string.c
int             memcmp(const void*, const void*, uint32_t);
void*           memmove(void*, const void*, uint32_t);
void*           memset(void*, int, uint32_t);
char*           safestrcpy(char*, const char*, int);
int             strlen(const char*);
int             strncmp(const char*, const char*, uint32_t);
char*           strncpy(char*, const char*, int);

// syscall.c
int             argint(int, int*);
int             argstr(int, char*, int);
int             argaddr(int, uint64_t *);
int             argfd(int, int *, struct file **);
int             fetchstr(uint64_t, char*, int);
int             fetchaddr(uint64_t, uint64_t*);
void            syscall();

// trap.c
extern uint32_t ticks;
void            trapinit(void);
void            trapinithart(void);
extern struct spinlock tickslock;
void            usertrapret(void);

// uart.c
void            uartinit(void);
void            uartintr(void);
void            uartputc(int);
int             uartgetc(void);

// vm.c
void            kvminit(void);
void            kvminithart(void);
uint64_t          kvmpa(uint64_t);
void            kvmmap(uint64_t, uint64_t, uint64_t, int);
int             mappages(pagetable_t, uint64_t, uint64_t, uint64_t, int);
pagetable_t     uvmcreate(void);
void            uvminit(pagetable_t, uint8_t *, uint32_t);
uint64_t          uvmalloc(pagetable_t, uint64_t, uint64_t);
uint64_t          uvmdealloc(pagetable_t, uint64_t, uint64_t);
int             uvmcopy(pagetable_t, pagetable_t, uint64_t);
void            uvmfree(pagetable_t, uint64_t);
void            uvmunmap(pagetable_t, uint64_t, uint64_t, int);
void            uvmclear(pagetable_t, uint64_t);
uint64_t          walkaddr(pagetable_t, uint64_t);
int             copyout(pagetable_t, uint64_t, char *, uint64_t);
int             copyin(pagetable_t, char *, uint64_t, uint64_t);
int             copyinstr(pagetable_t, char *, uint64_t, uint64_t);

// plic.c
void            plicinit(void);
void            plicinithart(void);
uint64_t          plic_pending(void);
int             plic_claim(void);
void            plic_complete(int);

// virtio_disk.c
void            virtio_disk_init(void);
void            virtio_disk_rw(struct buf *, int);
void            virtio_disk_intr();

// pci.c
void            pci_init();

// e1000.c
void            e1000_init(uint32_t *);
void            e1000_intr();
int             e1000_transmit(struct mbuf *);

// net.c
void            net_rx(struct mbuf *);
void            net_tx_udp(struct mbuf *, uint32_t,
                            uint16_t, uint16_t);

// arp.c
void            arpinit();

// tcp.c
void            tcpinit();

// sysfile.c
int             fdalloc(struct file *);

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x)/sizeof((x)[0]))

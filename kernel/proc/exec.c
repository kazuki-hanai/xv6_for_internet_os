#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "elf.h"

static int loadseg(pde_t *pgdir, uint64_t addr, struct inode *ip, uint32_t offset, uint32_t sz);

int
exec(char *path, char **argv)
{
	char *s, *last;
	int i, off;
	uint64_t argc, sz, sp, ustack[MAXARG+1], stackbase;
	struct elfhdr elf;
	struct inode *ip;
	struct proghdr ph;
	pagetable_t pagetable = 0, oldpagetable;
	struct proc *p = myproc();

	begin_op();

	if((ip = namei(path)) == 0){
		end_op();
		return -1;
	}
	ilock(ip);

	// Check ELF header
	if(readi(ip, 0, (uint64_t)&elf, 0, sizeof(elf)) != sizeof(elf))
		goto bad;
	if(elf.magic != ELF_MAGIC)
		goto bad;

	if((pagetable = proc_pagetable(p)) == 0)
		goto bad;

	// Load program into memory.
	sz = 0;
	for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
		if(readi(ip, 0, (uint64_t)&ph, off, sizeof(ph)) != sizeof(ph))
			goto bad;
		if(ph.type != ELF_PROG_LOAD)
			continue;
		if(ph.memsz < ph.filesz)
			goto bad;
		if(ph.vaddr + ph.memsz < ph.vaddr)
			goto bad;
		if((sz = uvmalloc(pagetable, sz, ph.vaddr + ph.memsz)) == 0)
			goto bad;
		if(ph.vaddr % PGSIZE != 0)
			goto bad;
		if(loadseg(pagetable, ph.vaddr, ip, ph.off, ph.filesz) < 0)
			goto bad;
	}
	iunlockput(ip);
	end_op();
	ip = 0;

	p = myproc();
	uint64_t oldsz = p->sz;

	// Allocate two pages at the next page boundary.
	// Use the second as the user stack.
	sz = PGROUNDUP(sz);
	if((sz = uvmalloc(pagetable, sz, sz + 2*PGSIZE)) == 0)
		goto bad;
	uvmclear(pagetable, sz-2*PGSIZE);
	sp = sz;
	stackbase = sp - PGSIZE;

	// Push argument strings, prepare rest of stack in ustack.
	for(argc = 0; argv[argc]; argc++) {
		if(argc >= MAXARG)
			goto bad;
		sp -= strlen(argv[argc]) + 1;
		sp -= sp % 16; // riscv sp must be 16-byte aligned
		if(sp < stackbase)
			goto bad;
		if(copyout(pagetable, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
			goto bad;
		ustack[argc] = sp;
	}
	ustack[argc] = 0;

	// push the array of argv[] pointers.
	sp -= (argc+1) * sizeof(uint64_t);
	sp -= sp % 16;
	if(sp < stackbase)
		goto bad;
	if(copyout(pagetable, sp, (char *)ustack, (argc+1)*sizeof(uint64_t)) < 0)
		goto bad;

	// arguments to user main(argc, argv)
	// argc is returned via the system call return
	// value, which goes in a0.
	p->tf->a1 = sp;

	// Save program name for debugging.
	for(last=s=path; *s; s++)
		if(*s == '/')
			last = s+1;
	safestrcpy(p->name, last, sizeof(p->name));
		
	// Commit to the user image.
	oldpagetable = p->pagetable;
	p->pagetable = pagetable;
	p->sz = sz;
	p->tf->epc = elf.entry;  // initial program counter = main
	p->tf->sp = sp; // initial stack pointer
	proc_freepagetable(oldpagetable, oldsz);
	return argc; // this ends up in a0, the first argument to main(argc, argv)

 bad:
	if(pagetable)
		proc_freepagetable(pagetable, sz);
	if(ip){
		iunlockput(ip);
		end_op();
	}
	return -1;
}

// Load a program segment into pagetable at virtual address va.
// va must be page-aligned
// and the pages from va to va+sz must already be mapped.
// Returns 0 on success, -1 on failure.
static int
loadseg(pagetable_t pagetable, uint64_t va, struct inode *ip, uint32_t offset, uint32_t sz)
{
	uint32_t i, n;
	uint64_t pa;

	if((va % PGSIZE) != 0)
		panic("loadseg: va must be page aligned");

	for(i = 0; i < sz; i += PGSIZE){
		pa = walkaddr(pagetable, va + i);
		if(pa == 0)
			panic("loadseg: address should exist");
		if(sz - i < PGSIZE)
			n = sz - i;
		else
			n = PGSIZE;
		if(readi(ip, 0, (uint64_t)pa, offset+i, n) != n)
			return -1;
	}
	
	return 0;
}

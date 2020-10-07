#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "sys/syscall.h"
#include "lib/hashmap.h"

struct spinlock tickslock;
uint32_t ticks;


extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

static const char* scause_desc(uint64_t stval);

struct hashmap *devintr_callbacks;
typedef void (*devintr_callback)();
struct devintr_map {
	int irq;
	devintr_callback callback;
};
static uint64_t devintr_hash(const void *item, uint64_t seed0, uint64_t seed1);
static int      devintr_compare(const void* a, const void* b, void* udata);
static struct devintr_map* devintr_register_callback(int irq, devintr_callback callback);

void
trapinit(void)
{
	initlock(&tickslock, "time");
	hashmap_set_allocator(ufkalloc, ufkfree);
	if ((devintr_callbacks = hashmap_new(
		sizeof(struct devintr_map),
		0, 0, 0, devintr_hash, devintr_compare, 0)) == NULL)
	{
		panic("cannot init devintr_callbacks hashmap\n");
	}
	devintr_register_callback(UART0_IRQ, uartintr);
	devintr_register_callback(VIRTIO0_IRQ, virtio_disk_intr);
	devintr_register_callback(E1000_IRQ, e1000_intr);
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
	w_stvec((uint64_t)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void
usertrap(void)
{
	int which_dev = 0;

	if((r_sstatus() & SSTATUS_SPP) != 0)
		panic("usertrap: not from user mode");

	// send interrupts and exceptions to kerneltrap(),
	// since we're now in the kernel.
	w_stvec((uint64_t)kernelvec);

	struct proc *p = myproc();
	
	// save user program counter.
	p->tf->epc = r_sepc();
	
	if(r_scause() == 8){
		// system call

		if(p->killed)
			exit(-1);

		// sepc points to the ecall instruction,
		// but we want to return to the next instruction.
		p->tf->epc += 4;

		// an interrupt will change sstatus &c registers,
		// so don't enable until done with those registers.
		intr_on();

		syscall();
	} else if((which_dev = devintr()) != 0){
		// ok
	} else {
		printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
		printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
		p->killed = 1;
	}

	if(p->killed)
		exit(-1);

	// give up the CPU if this is a timer interrupt.
	if(which_dev == 2)
		yield();

	usertrapret();
}

//
// return to user space
//
void
usertrapret(void)
{
	struct proc *p = myproc();

	// turn off interrupts, since we're switching
	// now from kerneltrap() to usertrap().
	intr_off();

	// send syscalls, interrupts, and exceptions to trampoline.S
	w_stvec(TRAMPOLINE + (uservec - trampoline));

	// set up trapframe values that uservec will need when
	// the process next re-enters the kernel.
	p->tf->kernel_satp = r_satp();         // kernel page table
	p->tf->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
	p->tf->kernel_trap = (uint64_t)usertrap;
	p->tf->kernel_hartid = r_tp();         // hartid for cpuid()

	// set up the registers that trampoline.S's sret will use
	// to get to user space.
	
	// set S Previous Privilege mode to User.
	unsigned long x = r_sstatus();
	x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
	x |= SSTATUS_SPIE; // enable interrupts in user mode
	w_sstatus(x);

	// set S Exception Program Counter to the saved user pc.
	w_sepc(p->tf->epc);

	// tell trampoline.S the user page table to switch to.
	uint64_t satp = MAKE_SATP(p->pagetable);

	// jump to trampoline.S at the top of memory, which 
	// switches to the user page table, restores user registers,
	// and switches to user mode with sret.
	uint64_t fn = TRAMPOLINE + (userret - trampoline);
	((void (*)(uint64_t,uint64_t))fn)(TRAPFRAME, satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
// must be 4-byte aligned to fit in stvec.
void 
kerneltrap()
{
	int which_dev = 0;
	uint64_t sepc = r_sepc();
	uint64_t sstatus = r_sstatus();
	uint64_t scause = r_scause();
	
	if((sstatus & SSTATUS_SPP) == 0)
		panic("kerneltrap: not from supervisor mode");
	if(intr_get() != 0)
		panic("kerneltrap: interrupts enabled");

	if((which_dev = devintr()) == 0){
		printf("scause %p (%s)\n", scause, scause_desc(scause));
		printf("sepc=%p stval=%p\n", r_sepc(), r_stval());
		panic("kerneltrap");
	}

	// give up the CPU if this is a timer interrupt.
	if(which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
		yield();

	// the yield() may have caused some traps to occur,
	// so restore trap registers for use by kernelvec.S's sepc instruction.
	w_sepc(sepc);
	w_sstatus(sstatus);
}

void
clockintr()
{
	acquire(&tickslock);
	ticks++;
	wakeup(&ticks);
	release(&tickslock);
}

static uint64_t devintr_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const struct devintr_map* m = item;
	return hashmap_sip(&m->irq, sizeof(m->irq), 0, 0);
}
static int devintr_compare(const void* a, const void* b, void* udata) {
	const struct devintr_map* ma = a;
	const struct devintr_map* mb = b;
	return ma->irq != mb->irq;
}
static struct devintr_map* devintr_register_callback(int irq, devintr_callback callback) {
	return (struct devintr_map*)hashmap_set(
		devintr_callbacks,
		&(struct devintr_map){ .irq = irq, .callback = callback});
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
	uint64_t scause = r_scause();

	if((scause & 0x8000000000000000L) &&
		 (scause & 0xff) == 9){
		// this is a supervisor external interrupt, via PLIC.

		// irq indicates which device interrupted.
		int irq = plic_claim();

		struct devintr_map* map = hashmap_get(
			devintr_callbacks,
			&(struct devintr_map){ .irq = irq });
		if (map != NULL) {
			map->callback();
		}

		plic_complete(irq);
		return 1;
	} else if(scause == 0x8000000000000001L){
		// software interrupt from a machine-mode timer interrupt,
		// forwarded by timervec in kernelvec.S.

		if(cpuid() == 0){
			clockintr();
		}
		
		// acknowledge the software interrupt by clearing
		// the SSIP bit in sip.
		w_sip(r_sip() & ~2);

		return 2;
	} else {
		return 0;
	}
}

static const char *
scause_desc(uint64_t stval)
{
	static const char *intr_desc[16] = {
		[0] "user software interrupt",
		[1] "supervisor software interrupt",
		[2] "<reserved for future standard use>",
		[3] "<reserved for future standard use>",
		[4] "user timer interrupt",
		[5] "supervisor timer interrupt",
		[6] "<reserved for future standard use>",
		[7] "<reserved for future standard use>",
		[8] "user external interrupt",
		[9] "supervisor external interrupt",
		[10] "<reserved for future standard use>",
		[11] "<reserved for future standard use>",
		[12] "<reserved for future standard use>",
		[13] "<reserved for future standard use>",
		[14] "<reserved for future standard use>",
		[15] "<reserved for future standard use>",
	};
	static const char *nointr_desc[16] = {
		[0] "instruction address misaligned",
		[1] "instruction access fault",
		[2] "illegal instruction",
		[3] "breakpoint",
		[4] "load address misaligned",
		[5] "load access fault",
		[6] "store/AMO address misaligned",
		[7] "store/AMO access fault",
		[8] "environment call from U-mode",
		[9] "environment call from S-mode",
		[10] "<reserved for future standard use>",
		[11] "<reserved for future standard use>",
		[12] "instruction page fault",
		[13] "load page fault",
		[14] "<reserved for future standard use>",
		[15] "store/AMO page fault",
	};
	uint64_t interrupt = stval & 0x8000000000000000L;
	uint64_t code = stval & ~0x8000000000000000L;
	if (interrupt) {
		if (code < NELEM(intr_desc)) {
			return intr_desc[code];
		} else {
			return "<reserved for platform use>";
		}
	} else {
		if (code < NELEM(nointr_desc)) {
			return nointr_desc[code];
		} else if (code <= 23) {
			return "<reserved for future standard use>";
		} else if (code <= 31) {
			return "<reserved for custom use>";
		} else if (code <= 47) {
			return "<reserved for future standard use>";
		} else if (code <= 63) {
			return "<reserved for custom use>";
		} else {
			return "<reserved for future standard use>";
		}
	}
}

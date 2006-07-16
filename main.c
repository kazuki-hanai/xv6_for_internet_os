#include "types.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "traps.h"
#include "syscall.h"
#include "elf.h"
#include "param.h"
#include "spinlock.h"

extern char edata[], end[];
extern int acpu;
extern uint8_t _binary_user1_start[], _binary_user1_size[];
extern uint8_t _binary_usertests_start[], _binary_usertests_size[];
extern uint8_t _binary_userfs_start[], _binary_userfs_size[];

extern int use_console_lock;

struct spinlock sillylock;  // hold this to keep interrupts disabled

int
main()
{
  struct proc *p;

  if (acpu) {
    cprintf("an application processor\n");
    idtinit(); // CPU's idt
    lapic_init(cpu());
    lapic_timerinit();
    lapic_enableintr();
    scheduler();
  }
  acpu = 1;

  // clear BSS
  memset(edata, 0, end - edata);

  mp_init(); // collect info about this machine

  acquire(&sillylock);
  use_console_lock = 1;

  lapic_init(mp_bcpu());

  cprintf("\nxV6\n\n");

  pic_init(); // initialize PIC
  kinit(); // physical memory allocator
  tvinit(); // trap vectors
  idtinit(); // CPU's idt

  // create fake process zero
  p = &proc[0];
  memset(p, 0, sizeof *p);
  p->state = SLEEPING;
  p->sz = 4 * PAGE;
  p->mem = kalloc(p->sz);
  memset(p->mem, 0, p->sz);
  p->kstack = kalloc(KSTACKSIZE);
  p->tf = (struct Trapframe *) (p->kstack + KSTACKSIZE - sizeof(struct Trapframe));
  memset(p->tf, 0, sizeof(struct Trapframe));
  p->tf->es = p->tf->ds = p->tf->ss = (SEG_UDATA << 3) | 3;
  p->tf->cs = (SEG_UCODE << 3) | 3;
  p->tf->eflags = FL_IF;
  p->pid = 0;
  p->ppid = 0;
  setupsegs(p);

  mp_startthem();

  // turn on timer and enable interrupts on the local APIC
  lapic_timerinit();
  lapic_enableintr();

  // init disk device
  //ide_init(); 

  // become interruptable
  sti();

  p = copyproc(&proc[0]);
  
  load_icode(p, _binary_usertests_start, (unsigned) _binary_usertests_size);
  //load_icode(p, _binary_userfs_start, (unsigned) _binary_userfs_size);
  p->state = RUNNABLE;
  cprintf("loaded userfs\n");
  release(&sillylock);

  scheduler();

  return 0;
}

void
load_icode(struct proc *p, uint8_t *binary, unsigned size)
{
  int i;
  struct Elf *elf;
  struct Proghdr *ph;

  // Check magic number on binary
  elf = (struct Elf*) binary;
  cprintf("elf %x magic %x\n", elf, elf->magic);
  if (elf->magic != ELF_MAGIC)
    panic("load_icode: not an ELF binary");

  p->tf->eip = elf->entry;
  p->tf->esp = p->sz;

  // Map and load segments as directed.
  ph = (struct Proghdr*) (binary + elf->phoff);
  for (i = 0; i < elf->phnum; i++, ph++) {
    if (ph->type != ELF_PROG_LOAD)
      continue;
    cprintf("va %x memsz %d\n", ph->va, ph->memsz);
    if (ph->va + ph->memsz < ph->va)
      panic("load_icode: overflow in elf header segment");
    if (ph->va + ph->memsz >= p->sz)
      panic("load_icode: icode wants to be above UTOP");

    // Load/clear the segment
    memmove(p->mem + ph->va, binary + ph->offset, ph->filesz);
    memset(p->mem + ph->va + ph->filesz, 0, ph->memsz - ph->filesz);
  }
}

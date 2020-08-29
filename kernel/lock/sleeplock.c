// Sleeping locks

#include "types.h"
#include "arch/riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sleeplock.h"

void
initsleeplock(struct sleeplock *lk, char *name)
{
  initlock(&lk->lk, "sleep lock");
  lk->name = name;
  lk->locked = 0;
  lk->pid = 0;
}

void
acquiresleep(struct sleeplock *lk)
{
  acquire(&lk->lk);
  while (lk->locked) {
    sleep(lk, &lk->lk);
  }
  lk->locked = 1;
  struct proc *proc = myproc();
  if (proc == 0) {
    lk->pid = -1;
  } else {
    lk->pid = proc->pid;
  }
  release(&lk->lk);
}

void
releasesleep(struct sleeplock *lk)
{
  acquire(&lk->lk);
  lk->locked = 0;
  lk->pid = 0;
  wakeup(lk);
  release(&lk->lk);
}

int
holdingsleep(struct sleeplock *lk)
{
  int r;
  
  acquire(&lk->lk);
  // for kernel test
  if (lk->pid == -1)
    r = lk->locked && cpuid() == 0;
  else
    r = lk->locked && (lk->pid == myproc()->pid);
  release(&lk->lk);
  return r;
}
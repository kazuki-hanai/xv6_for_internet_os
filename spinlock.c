// Mutual exclusion spin locks.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

void
initlock(struct spinlock *lock, char *name)
{
  lock->name = name;
  lock->locked = 0;
  lock->cpu = 0xffffffff;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void
acquire(struct spinlock *lock)
{
  pushcli();
  if(holding(lock))
    panic("acquire");

  // The xchg is atomic.
  // It also serializes, so that reads after acquire are not
  // reordered before it.  
  while(xchg(&lock->locked, 1) == 1)
    ;

  // Record info about lock acquisition for debugging.
  // The +10 is only so that we can tell the difference
  // between forgetting to initialize lock->cpu
  // and holding a lock on cpu 0.
  lock->cpu = cpu() + 10;
  getcallerpcs(&lock, lock->pcs);
}

// Release the lock.
void
release(struct spinlock *lock)
{
  if(!holding(lock))
    panic("release");

  lock->pcs[0] = 0;
  lock->cpu = 0xffffffff;

  // The xchg serializes, so that reads before release are 
  // not reordered after it.  The 1996 PentiumPro manual (Volume 3,
  // 7.2) says reads can be carried out speculatively and in
  // any order, which implies we need to serialize here.
  // But the 2007 Intel 64 Architecture Memory Ordering White
  // Paper says that Intel 64 and IA-32 will not move a load
  // after a store. So lock->locked = 0 would work here.
  // The xchg being asm volatile ensures gcc emits it after
  // the above assignments (and after the critical section).
  xchg(&lock->locked, 0);

  popcli();
}

// Record the current call stack in pcs[] by following the %ebp chain.
void
getcallerpcs(void *v, uint pcs[])
{
  uint *ebp;
  int i;
  
  ebp = (uint*)v - 2;
  for(i = 0; i < 10; i++){
    if(ebp == 0 || ebp == (uint*)0xffffffff)
      break;
    pcs[i] = ebp[1];     // saved %eip
    ebp = (uint*)ebp[0]; // saved %ebp
  }
  for(; i < 10; i++)
    pcs[i] = 0;
}

// Check whether this cpu is holding the lock.
int
holding(struct spinlock *lock)
{
  return lock->locked && lock->cpu == cpu() + 10;
}


// Pushcli/popcli are like cli/sti except that they are matched:
// it takes two popcli to undo two pushcli.  Also, if interrupts
// are off, then pushcli, popcli leaves them off.

void
pushcli(void)
{
  int eflags;
  
  eflags = readeflags();
  cli();
  if(c->ncli++ == 0)
    c->intena = eflags & FL_IF;
}

void
popcli(void)
{
  if(readeflags()&FL_IF)
    panic("popcli - interruptible");
  if(--c->ncli < 0)
    panic("popcli");
  if(c->ncli == 0 && c->intena)
    sti();
}


#pragma once

#include "types.h"

// Mutual exclusion lock.
struct spinlock {
  uint32_t locked;       // Is the lock held?

  // For debugging:
  char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
};


#pragma once

#include "spinlock.h"
#include "net/sock_cb.h"

// util
uint16 cksum16(uint8 *, uint16, uint32);
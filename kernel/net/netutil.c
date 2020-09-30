#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "net/byteorder.h"
#include "net/mbuf.h"
#include "net/netutil.h"
#include "net/ethernet.h"

uint16_t cksum16(uint8_t *data, uint16_t size, uint32_t init) {
	uint32_t sum;
	sum = init;
	while(size > 1) {
		sum += (*(data++)) << 8;
		sum += (*(data++));
		size -= 2;
	}
	if (size) {
		sum += (*data) << 8;
	}
	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);
	return ~sum;
}

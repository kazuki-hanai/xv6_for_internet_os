#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "arch/riscv.h"
#include "defs.h"

//
// the riscv Platform Level Interrupt Controller (PLIC).
//

void
plicinit(void)
{
	// set desired IRQ priorities non-zero (otherwise disabled).
	*(uint32_t*)(PLIC + UART0_IRQ*4) = 1;
	*(uint32_t*)(PLIC + VIRTIO0_IRQ*4) = 1;

	for(int irq = 1; irq < 0x35; irq++) {
		*(uint32_t*)(PLIC + irq*4) = 1;
	}
}

void
plicinithart(void)
{
	int hart = cpuid();
	
	// set uart's enable bit for this hart's S-mode. 
	*(uint32_t*)PLIC_SENABLE(hart)= (1 << UART0_IRQ) | (1 << VIRTIO0_IRQ);

	// hack to get at next 32 IRQs for e1000
	*(uint32_t*)(PLIC_SENABLE(hart)+4) = 0xffffffff;

	// set this hart's S-mode priority threshold to 0.
	*(uint32_t*)PLIC_SPRIORITY(hart) = 0;
}

// return a bitmap of which IRQs are waiting
// to be served.
uint64_t
plic_pending(void)
{
	uint64_t mask;

	//mask = *(uint32_t*)(PLIC + 0x1000);
	//mask |= (uint64_t)*(uint32_t*)(PLIC + 0x1004) << 32;
	mask = *(uint64_t*)PLIC_PENDING;

	return mask;
}

// ask the PLIC what interrupt we should serve.
int
plic_claim(void)
{
	int hart = cpuid();
	//int irq = *(uint32_t*)(PLIC + 0x201004);
	int irq = *(uint32_t*)PLIC_SCLAIM(hart);
	return irq;
}

// tell the PLIC we've served this IRQ.
void
plic_complete(int irq)
{
	int hart = cpuid();
	//*(uint32_t*)(PLIC + 0x201004) = irq;
	*(uint32_t*)PLIC_SCLAIM(hart) = irq;
}

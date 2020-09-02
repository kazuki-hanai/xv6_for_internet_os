#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "arch/riscv.h"
#include "defs.h"
#include "lib/buddy.h"
#include "lib/list.h"

#define MAX(a, b)	((a > b) ? a : b)

#define LEAF_SIZE		8 * 1024
#define NSIZES			7
#define MAX_SIZE		NSIZES - 1
#define BLK_SIZE(k)		(((1L << (k)) * LEAF_SIZE))
#define BIGPG_SIZE		(BLK_SIZE(MAX_SIZE))
#define NCHAR	 		((1 << NSIZES) / 8)
#define BIGPGROUNDUP(sz)	(((sz)+BIGPG_SIZE-1) & ~(BIGPG_SIZE-1))
#define NBIGPGS			128

extern char end[]; // first address after kernel. defined by kernel.ld.

struct pagelist {
	void*	pageaddr;
	char	alloc[NSIZES][NCHAR];
	char	split[NSIZES][NCHAR];
};

struct {
	struct spinlock lock;
	struct list 	freelist[NSIZES];
	struct pagelist plist[NBIGPGS];
} ufk_table;

static void table_init(void* pa_start, void* pa_end) {
	memset(ufk_table.plist, 0, sizeof(ufk_table.plist));
	for (int i = 0; i < NSIZES; i++) {
		lst_init(&ufk_table.freelist[i]);
	}

	void* p = (void*)BIGPGROUNDUP((uint64_t)pa_start);
	for(int i = 0; p + BIGPG_SIZE <= pa_end; p += BIGPG_SIZE, i++) {
		ufk_table.plist[i].pageaddr = p;
	}
}

void ufk_init() {
	initlock(&ufk_table.lock, "ufk_table");
	table_init(kalloc(), (void*)PHYSTOP);
}

static int bit_isset(char *map, int index) {
	char b = map[index/8];
	char m = (1 << (index % 8));
	return (b & m) == m;
}

static void bit_set(char *map, int index) {
	char b = map[index/8];
	char m = (1 << (index % 8));
	map[index/8] = (b | m);
}

static void bit_clear(char *map, int index) {
	char b = map[index/8];
	char m = (1 << (index % 8));
	map[index/8] = (b & ~m);
}

static int firstk(int n) {
	int k = 0;
	int size = LEAF_SIZE;
	while (size < n) {
		k++;
		size *= 2;
	}
	return k;
}

static void* _ufk_alloc(int nbytes) {
	acquire(&ufk_table.lock);
	
	int fk = firstk(nbytes);
	int k = fk;
	for (; k < NSIZES; k++) {
		if (!lst_empty(&ufk_table.freelist[k]))
			break;
	}
	if (k >= NSIZES)
		return 0;

	return 0;
}

void* ufk_alloc(int nbytes) {
	void* p = 0;
	if (nbytes < PGSIZE) {
		p = bd_alloc(nbytes);
		if (p == 0) {
			void* p = ufk_alloc(4096);
			bd_addpage(p);
			p = bd_alloc(nbytes);
		}
	} else {
		p = _ufk_alloc(nbytes);
		if (p == 0)
			panic("[ufk_alloc] could not allocate\n");
	}

	return p;
}

void ufk_free(void* p) {
	bit_clear(0, 0);
	bit_isset(0, 0);
	bit_set(0, 0);
}

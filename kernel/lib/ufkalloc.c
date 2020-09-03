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
		lst_push(&ufk_table.freelist[MAX_SIZE], ufk_table.plist[i].pageaddr);
	}
}

void ufkinit() {
	table_init(kalloc(), (void*)PHYSTOP);
	initlock(&ufk_table.lock, "ufk_table");
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

static int get_pageindex(void *p) {
	for (int i = 0; i < NBIGPGS; i++) {
		if (ufk_table.plist[i].pageaddr == 0)
			break;
		int offset = p - ufk_table.plist[i].pageaddr;
		if (0 <= offset && offset < BIGPG_SIZE) {
			return i;
		}
	}
	return -1;
}

static int blk_index(int k, void* p, void* base) {
	int n = p - base;
	return n / BLK_SIZE(k);
}

static int blksize_index(void *p, int pindex, void* base) {
	int k;
	for (k = 0; k < NSIZES; k++) {
		if (bit_isset(ufk_table.plist[pindex].split[k+1], blk_index(k+1, p, base))) {
			return k;
		}
	}
	return k-1;
}

static void* addr(int k, int bi, void* base) {
	int n = bi * BLK_SIZE(k);
	return base + n;
}

static void* _ufkalloc(int nbytes) {
	int fk = firstk(nbytes);
	int k = fk;
	for (; k < NSIZES; k++) {
		if (!lst_empty(&ufk_table.freelist[k]))
			break;
	}
	if (k >= NSIZES)
		return 0;

	void* p = lst_pop(&ufk_table.freelist[k]);
	if (p == 0)
		panic("[_ufk_alloc] there are no rest memory");
	int pindex;
	if ((pindex = get_pageindex(p)) < 0)
		panic("[_ufk_alloc] cannot get page index");
	void* pbase = ufk_table.plist[pindex].pageaddr;	
	bit_set(ufk_table.plist[pindex].alloc[k], blk_index(k, p, pbase));
	for (; k > fk; k--) {
		void *q = p + BLK_SIZE(k-1);
		bit_set(ufk_table.plist[pindex].split[k], blk_index(k, p, pbase));
		bit_set(ufk_table.plist[pindex].alloc[k-1], blk_index(k-1, p, pbase));
		lst_push(&ufk_table.freelist[k-1], q);
	}

	return p;
}

void* ufkalloc(int nbytes) {
	void* p = 0;
	if (nbytes < PGSIZE) {
		p = bd_alloc(nbytes);
		if (p == 0) {
			// TODO: bit set
			void* p = ufkalloc(PGSIZE);
			int leafindex = 0;
			int pindex;
			if ((pindex = get_pageindex(p)) < 0)
				panic("[ufkalloc] cannot get page index");
			void *pbase = ufk_table.plist[pindex].pageaddr;
			bit_set(ufk_table.plist[pindex].split[leafindex], blk_index(leafindex, p, pbase));
			bd_addpage(p);
			bd_addpage(p+PGSIZE);
			p = bd_alloc(nbytes);
			if (p == 0)
				panic("[ufkalloc] cannot alloc...");
			return p;
		}
	} else {
		acquire(&ufk_table.lock);
		p = _ufkalloc(nbytes);
		release(&ufk_table.lock);
		if (p == 0)
			panic("[ufkalloc] could not allocate\n");
	}

	return p;
}

static void _ufkfree(void* p, int pindex, void* pbase) {
	acquire(&ufk_table.lock);
	
	int k;
	for (k = blksize_index(p, pindex, pbase); k < MAX_SIZE; k++) {
		int bi = blk_index(k, p, pbase);
		bit_clear(ufk_table.plist[pindex].alloc[k], bi);
		int buddy = (bi % 2 == 0) ? bi+1 : bi-1;
		if (bit_isset(ufk_table.plist[pindex].alloc[k], buddy)) {
			break;
		}
		void* q = addr(k, buddy, pbase);
		lst_remove(q);
		if (buddy % 2 == 0) {
			p = q;
		}
		bit_clear(ufk_table.plist[pindex].alloc[k+1], blk_index(k+1, p, pbase));
		bit_clear(ufk_table.plist[pindex].split[k+1], blk_index(k+1, p, pbase));
	}
	lst_push(&ufk_table.freelist[k], p);
	release(&ufk_table.lock);
}
void ufkfree(void* p) {
	int leafindex = 0;
	int pindex;
	if ((pindex = get_pageindex(p)) < 0)
		panic("[uffree] cannot get page index");
	void *pbase = ufk_table.plist[pindex].pageaddr;
	
	if (bit_isset(ufk_table.plist[pindex].split[leafindex], blk_index(leafindex, p, pbase))) {
		bd_free(p);
	} else {
		_ufkfree(p, pindex, pbase);
	}
}

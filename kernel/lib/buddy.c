#include "types.h"
#include "param.h"
#include "arch/riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "lib/buddy.h"
#include "lib/list.h"

#define LEAF_SIZE 	32
#define NSIZES		8
#define MAX_SIZE	NSIZES-1
#define BLK_SIZE(k)	((1L << (k)) * LEAF_SIZE) 
#define HEAP_SIZE	BLK_SIZE(MAX_SIZE) // 4096
#define MAX_PAGES	128
#define MAX_FREED 	(PGSIZE * MAX_PAGES)

struct pagelist {
	void *pageaddr;
	char alloc[NSIZES][16];
	char split[NSIZES][16];
};

struct {
	struct spinlock lock;
	struct list freelist[NSIZES];
	struct pagelist plist[MAX_PAGES];
} bd_table;

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

void
bd_init()
{
	initlock(&bd_table.lock, "buddytable");
	for (int i = 0; i < MAX_PAGES; i++) {
		memset(&bd_table.plist[i], 0, sizeof(bd_table.plist[i]));
	}

	for (int i = 0; i < NSIZES; i++) {
		lst_init(&bd_table.freelist[i]);
	}
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

static int get_page_index(void *addr) {
	for (int i = 0; i < MAX_PAGES; i++) {
		int offset = addr - bd_table.plist[i].pageaddr;
		if (0 <= offset && offset < HEAP_SIZE) {
			return i;
		}
	}
	return -1;
}

static int blk_index(int k, char *p, void *base_addr) {
	int n = p - (char *) base_addr;
	return n / BLK_SIZE(k);
}

static int blk_size(void *p, int pindex, void* base_addr) {
	int k;
	for (k = 0; k < NSIZES; k++) {
		if(bit_isset(bd_table.plist[pindex].split[k+1], blk_index(k+1, p, base_addr))) {
			return k;
		}
	}
	return k-1;
}

static void *addr(int k, int bi, void* base_addr) {
	int n = bi * BLK_SIZE(k);
	return base_addr + n;
}

void bd_addpage(void *p) {
	acquire(&bd_table.lock);
	for (int i = 0; i < MAX_PAGES; i++) {
		if (bd_table.plist[i].pageaddr == 0) {
			bd_table.plist[i].pageaddr = p;
			lst_push(&bd_table.freelist[MAX_SIZE], bd_table.plist[i].pageaddr);
			release(&bd_table.lock);
			return;
		}
	}
	panic("[bd_addpage] cannot add new page");
}

void
bd_free(void *p)
{
	void *q;
	int k;
	int pindex = get_page_index(p);
	void *pbase = bd_table.plist[pindex].pageaddr;
	acquire(&bd_table.lock);
	k = blk_size(p, pindex, pbase);
	for (k = blk_size(p, pindex, pbase); k < MAX_SIZE; k++) {
		int bi = blk_index(k, p, pbase);
		bit_clear(bd_table.plist[pindex].alloc[k], bi);
		int buddy = (bi % 2 == 0) ? bi+1 : bi-1;
		if (bit_isset(bd_table.plist[pindex].alloc[k], buddy)) {
			break;
		}
		q = addr(k, buddy, pbase);
		lst_remove(q);
		if (buddy % 2 == 0) {
			p = q;
		}
		bit_clear(bd_table.plist[pindex].alloc[k+1], blk_index(k+1, p, pbase));
		bit_clear(bd_table.plist[pindex].split[k+1], blk_index(k+1, p, pbase));
	}
	lst_push(&bd_table.freelist[k], p);
	release(&bd_table.lock);
}

void* bd_alloc(int nbytes) {
	int fk, k;

	acquire(&bd_table.lock);
	fk = firstk(nbytes);
	for (k = fk; k < NSIZES; k++) {
		if(!lst_empty(&bd_table.freelist[k]))
			break;
	}
	if (k >= NSIZES) {
		release(&bd_table.lock);
		return 0;
	}

	char *p = lst_pop(&bd_table.freelist[k]);
	if (p == 0) {
		panic("[bd_alloc] there are no rest memory");
	}
	int pindex = get_page_index(p);
	void *pbase = bd_table.plist[pindex].pageaddr;
	bit_set(bd_table.plist[pindex].alloc[k], blk_index(k, p, pbase));
	for (; k > fk; k--) {
		char *q = p + BLK_SIZE(k-1);
		bit_set(bd_table.plist[pindex].split[k], blk_index(k, p, pbase));
		bit_set(bd_table.plist[pindex].alloc[k-1], blk_index(k-1, p, pbase));
		lst_push(&bd_table.freelist[k-1], q);
	}
	release(&bd_table.lock);

	return p;
}

void buddy_show_map() {
	for (int i = 0; i < MAX_PAGES; i++) {
		printf("\npage: %d\n", i);
		printf("\nalloc\n");
		for (int j = 0; j < NSIZES; j++) {
			printf("\tsize: %d\n\t", LEAF_SIZE << j);
			int th = (16 >> j) > 0 ? (16 >> j) : 1;
			for (int p = 0; p < th; p++) {
				printf("%x ", (bd_table.plist[i].alloc[j][p]) & 0xf);
				printf("%d ", (bd_table.plist[i].alloc[j][p] >> 4) & 0xf);
			}
			printf("\n");
		}
		printf("\nsplit\n");
		for (int j = 0; j < NSIZES; j++) {
			printf("\tsize: %d\n\t", LEAF_SIZE << j);
			int th = (16 >> j) > 0 ? (16 >> j) : 1;
			for (int p = 0; p < th; p++) {
				printf("%x ", (bd_table.plist[i].split[j][p]) & 0xf);
				printf("%d ", (bd_table.plist[i].split[j][p] >> 4) & 0xf);
			}
			printf("\n");
		}
	}
}

#include "user.h"

enum {
	NHASH = 128
};

struct intlist
{
	uint64_t id;
	void*	aux;
	struct intlist*	link;
};

struct intmap
{
	// lock;
	struct intlist*	hash[NHASH];
	void            (*inc)(void*);
};

static uint64_t
hashid(uint64_t id)
{
	return id%NHASH;
}

static void
nop(void *v)
{
	((void)(v));
}

struct intmap*
allocmap(void (*inc)(void*))
{
	struct intmap *m;

	m = malloc(sizeof(*m));
	if (m == 0) {
		return 0;
	}
	if(inc == 0)
		inc = nop;
	m->inc = inc;
	return m;
}

void
freemap(struct intmap *map, void (*destroy)(void*))
{
	int i;
	struct intlist *p, *nlink;

	if(destroy == 0)
		destroy = nop;
	for(i=0; i<NHASH; i++){
		for(p=map->hash[i]; p; p=nlink){
			nlink = p->link;
			destroy(p->aux);
			free(p);
		}
	}

	free(map);
}

static struct intlist**
llookup(struct intmap *map, uint64_t id)
{
	struct intlist **lf;

	for(lf=&map->hash[hashid(id)]; *lf; lf=&(*lf)->link)
		if((*lf)->id == id)
			break;
	return lf;
}

/*
 * The RWlock is used as expected except that we allow
 * inc() to be called while holding it.  This is because we're
 * locking changes to the tree structure, not to the references.
 * Inc() is expected to have its own locking.
 */
void*
lookupkey(struct intmap *map, uint64_t id)
{
	struct intlist *f;
	void *v;

	// rlock(&map->rwlock);
	if((f = *llookup(map, id))){
		v = f->aux;
		map->inc(v);
	}else
		v = 0;
	// runlock(&map->rwlock);
	return v;
}

void*
insertkey(struct intmap *map, uint64_t id, void *v)
{
	struct intlist *f;
	void *ov;
	uint64_t h;

	// wlock(&map->rwlock);
	if((f = *llookup(map, id))){
		/* no decrement for ov because we're returning it */
		ov = f->aux;
		f->aux = v;
	}else{
		f = malloc(sizeof(*f));
		f->id = id;
		f->aux = v;
		h = hashid(id);
		f->link = map->hash[h];
		map->hash[h] = f;
		ov = 0;
	}
	// wunlock(&map->rwlock);
	return ov;
}

int
caninsertkey(struct intmap *map, uint64_t id, void *v)
{
	struct intlist *f;
	int rv;
	uint64_t h;

	// wlock(&map->rwlock);
	if(*llookup(map, id))
		rv = 0;
	else{
		f = malloc(sizeof *f);
		f->id = id;
		f->aux = v;
		h = hashid(id);
		f->link = map->hash[h];
		map->hash[h] = f;
		rv = 1;
	}
	// wunlock(&map->rwlock);
	return rv;
}

void*
deletekey(struct intmap *map, uint64_t id)
{
	struct intlist **lf, *f;
	void *ov;

	// wlock(&map->rwlock);
	if((f = *(lf = llookup(map, id)))){
		ov = f->aux;
		*lf = f->link;
		free(f);
	}else
		ov = 0;
	// wunlock(&map->rwlock);
	return ov;
}


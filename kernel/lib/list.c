#include "types.h"
#include "param.h"
#include "defs.h"
#include "lib/list.h"

void lst_init(struct list *lst) {
	lst->next = lst;
	lst->prev = lst;
}

int lst_empty(struct list *lst) {
	return lst->next == lst;
}

 void lst_remove(struct list *e) {
	if (e == 0) {
		panic("[lst_remove] a list has no member");
	} else if (e->prev == 0) {
		panic("[lst_remove] a list prev has no member");
	} else if (e->next == 0) {
		panic("a list next has no member");
	}
	e->prev->next = e->next;
	e->next->prev = e->prev;
}

void* lst_pop(struct list *lst) {
	struct list *p = lst->next;
	if (p == 0) {
		return 0;
	}
	lst_remove(p);
	return (void *)p;
}

void lst_push(struct list *lst, void *p) {
	struct list *e = (struct list *) p;
	if (lst == 0)
		panic("[lst_push] list did not allocated\n");
	if (e == 0)
		panic("[lst_push] p did not allocated\n");
	e->next = lst->next;
	e->prev = lst;
	lst->next->prev = p;
	lst->next = e;
}

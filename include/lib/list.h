#pragma once

struct list {
	struct list *prev;
	struct list *next;
};

void	lst_init(struct list*);
void	lst_remove(struct list*);
void	lst_push(struct list*, void *);
void*	lst_pop(struct list*);
int	lst_empty(struct list*);

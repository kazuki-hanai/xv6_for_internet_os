#pragma once

void*	bd_alloc(int);
void 	bd_free(void *plist);
void 	bd_addpage(void *p);
void 	buddy_show_map();

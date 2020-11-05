#pragma once

#include "types.h"
#include <stdint.h>

struct stat;
struct rtcdate;
struct intmap;

// system calls
int fork(void);
int exit(int) __attribute__((noreturn));
int wait(int*);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int socket(int);
int connect(int, uint32_t, uint16_t);
int listen(int, uint16_t);
int accept(int, uint32_t*, uint16_t*);
uint64_t calc(int);
int getnodes(struct node_map* nm);
int addnode(uint64_t nid);
int removenode(uint64_t nid);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
int strncmp(const char*, const char*, uint32_t);
void fprintf(int, const char*, ...);
void printf(const char*, ...);
char* gets(char*, int max);
uint32_t strlen(const char*);
void* memset(void*, int, uint32_t);
void* malloc(uint32_t);
void free(void*);
int atoi(const char*);

// intmap
struct intmap;
struct intmap* allocmap(void (*inc)(void*));
void freemap(struct intmap*, void (*destroy)(void*));
void* lookupkey(struct intmap*, uint64_t id);
void* insertkey(struct intmap*, uint64_t id, void *v);
int caninsertkey(struct intmap*, uint64_t id, void *v);
void* deletekey(struct intmap*, uint64_t id);

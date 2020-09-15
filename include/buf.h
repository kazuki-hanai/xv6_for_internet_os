#pragma once
struct buf {
	int valid;   // has data been read from disk?
	int disk;    // does disk "own" buf?
	uint32_t dev;
	uint32_t blockno;
	struct sleeplock lock;
	uint32_t refcnt;
	struct buf *prev; // LRU cache list
	struct buf *next;
	struct buf *qnext; // disk queue
	uint8_t data[BSIZE];
};


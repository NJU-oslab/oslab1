#ifndef _PMM_H
#define _PMM_H

typedef union block {
	struct {
		union block* next;
		size_t size;
	}body;
	long align;
} Block;

#endif
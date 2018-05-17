#include "os.h"
#include <unistd.h>
#include <stdint.h>

static void pmm_init();
static void *pmm_alloc(size_t size);
static void pmm_free(void *ptr);

/*
static void *current;

struct _Block {
	size_t size; // Aligned size
	void *start; // the start of block area
	int free; // free = this block is occupied? 0 : 1;
	void *next; // the next block
}; 

#define BLOCK_SIZE 16

struct _Block *block;

MOD_DEF(pmm) {
	.init = pmm_init,
	.alloc = pmm_alloc,
	.free = pmm_free,
};

static void pmm_init() {
	current = _heap.start;
	block = NULL;
}

int digit(size_t size) {
		int d = 0;
		while (size != 0)
		{
			size = size >> 1;
			d += 1;
		}
		return d;
	};

	size_t align(int digit) {
		return 1 << (digit - 1);
	};

static void *pmm_alloc(size_t size) {

	if (size == 0)
	{
		return NULL;
	}
	//Log("New block to be created with size %d", size);
	if (block == NULL)
	{
		void *p = current;
		while (((size_t)p) % align(digit(size + BLOCK_SIZE)) != 0) p++;
		current = p + align(digit(size + BLOCK_SIZE));
		//Log("current = %8x and p = %8x", (unsigned)current, (unsigned)p);
		block = (struct _Block *)p + size;
		block->size = align(digit(size + BLOCK_SIZE)) - BLOCK_SIZE;
		block->start = p;
		block->free = 0;
		block->next = NULL;
		Log("ret: 0x%x", p);
		return p;
	}
	else
	{
		struct _Block *p = block;
		while (p->next != NULL)
		{
			if (p->size >= size && p->free){
				Log("ret: 0x%x", p->start);
				return p->start;
			}
			p = p->next;
		}
		if (p->size >= size && p->free) {
			Log("ret: 0x%x", p->start);
			return p->start;
		}
		void *q = current;
		while (((size_t)q) % align(digit(size + BLOCK_SIZE)) != 0) q++;
		current = q + align(digit(size + BLOCK_SIZE));
		block = (struct _Block *)p + size;
		p->size = align(digit(size + BLOCK_SIZE)) - BLOCK_SIZE;
		p->start = q;
		p->free = 0;
		p->next = NULL;
		Log("ret: 0x%x", p->start);
		return p->start;
	}
}

static void pmm_free(void *ptr) {
	struct _Block *p = block;
	while (p->next != NULL)
	{
		if (p->start == ptr)
		{
			p->free = 1;
			break;
		}
		p = p->next;
	}
	if (p->start == ptr)
		p->free = 1;
}*/


static spinlock_t pmm_lock;
static void *cur = NULL;
static size_t mul = 1;

MOD_DEF(pmm) {
    .init = pmm_init,
    .alloc = pmm_alloc,
    .free = pmm_free
};


typedef union block {
	struct {
		union block* next;
		size_t size;
	}body;
	long align;
} Block;

static Block base;
static Block *freep = NULL;

static void free_unsafe(void *ptr) {
	TRACE_ENTRY;
	Block* current, *head;
	head = (Block *)ptr - 1;
	current = freep;
	while (head <= current || head >= current->body.next){
		if (current >= current->body.next && (head > current || head < current->body.next))
			break;
		current = current->body.next;
	}
	//merge it and its prev
	if ((char*)head + head->body.size == (char*)current->body.next){
		head->body.size += current->body.next->body.size;
		head->body.next = current->body.next->body.next;
	}
	else
		head->body.next = current->body.next;
	//merge it and its next
	if ((char*)current + current->body.size == (char*)head) {
		current->body.size += head->body.size;
		current->body.next = head->body.next;
	}
	else
		current->body.next = head;
	freep = current;
	TRACE_EXIT;
}

static size_t align(size_t size) {
    size_t k = 0;
	mul = 1;
	while (mul < size) {
		mul <<= 1;
		k++;
	}
	printf("size: %d\t mul: %d\n", size, mul);
	return mul;
}

static Block* allocate_new(size_t size) {
	TRACE_ENTRY;
	void *new_mem;
	Block *new_block;
	printf("size: %d\n", size);
	size = align(size);
	if (size + cur > _heap.end)
		return NULL;
	else {
		new_mem = cur;
		cur += size;
	}
	Log("new_mem: 0x%x", new_mem);
	new_block = (Block *)new_mem;
	new_block->body.size = size;
	free_unsafe((void*)(new_block+1));
	TRACE_EXIT;
	return freep;
}

static void* malloc_unsafe(size_t size) {
	TRACE_ENTRY;
	Block *current, *prev;
	prev = freep;
	if (prev == NULL) {
		freep = prev = &base;
		base.body.next = freep;
		base.body.size = 0;
	}
	current = prev->body.next;
	while (1) {
		if (current->body.size >= size) {
			if (current->body.size > size) {
				current->body.size -= size;
				current = (Block *)((char *)current + current->body.size);
				current->body.size = size;
		//		size = current->body.size;
		//		prev->body.next = current->body.next;
		//		freep = prev;
			}
			else {
				prev->body.next = current->body.next;
				freep = prev;
			}
			TRACE_EXIT;
			return (void *)(current);
		}
		if (current == freep && (current = allocate_new(size)) == NULL){
			TRACE_EXIT;
			return NULL;
		}
		prev = current;
		current = current->body.next;
	}
}


static void pmm_init(){
	cur = _heap.start;
	kmt->spin_init(&pmm_lock, "pmm_lock");
}

static void *pmm_alloc(size_t size){
	TRACE_ENTRY;
	kmt->spin_lock(&pmm_lock);
	void *ret = malloc_unsafe(size);
	Log("malloc's ret: 0x%x", ret);
	Log("RET mod 2^k = %d", (size_t)ret %mul);
//	assert((size_t)ret % mul == 0);
	kmt->spin_unlock(&pmm_lock);
	TRACE_EXIT;
	return ret;
}

static void pmm_free(void *ptr){
	TRACE_ENTRY;
	kmt->spin_lock(&pmm_lock);
	free_unsafe(ptr);
	kmt->spin_unlock(&pmm_lock);
	TRACE_EXIT;
}

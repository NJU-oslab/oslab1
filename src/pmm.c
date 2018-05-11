#include <os.h>
#include <unistd.h>
#include <stdint.h>
/*
#define MINMALLOC (8 << 10)
#define HEAP_SIZE (64 * 1024 * 1024)

static void pmm_init();
static void *pmm_alloc(size_t size);
static void pmm_free(void *ptr);

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

static char heap[HEAP_SIZE];

static Block base;
static Block *freep = NULL;
extern void *sbrk(intptr_t increment);

static void free_unsafe(void *ptr) {
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
}

static Block* allocate_new(size_t size) {
	void *new_mem;
	Block *new_block;
	if (size < MINMALLOC)
		size = MINMALLOC;
	if ((new_mem = sbrk(size)) == (void*)-1)
		return NULL;
	new_block = (Block *)new_mem;
	new_block->body.size = size;
	free_unsafe((void*)(new_block+1));
	return freep;
}

static size_t auto_align(size_t size) {
	return ((size + sizeof(Block) - 1) / sizeof(Block)) * sizeof(Block) + sizeof(Block);
}

static void* malloc_unsafe(size_t size) {
	Block *current, *prev;
	size = auto_align(size);
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
				freep = prev;
			}
			else {
				prev->body.next = current->body.next;
				freep = prev;
			}
			return (void *)(current+1);
		}
		if (current == freep && (current = allocate_new(size)) == NULL){
			return NULL;
		}
		prev = current;
		current = current->body.next;
	}
}


static void pmm_init(){
}

static void *pmm_alloc(size_t size){
	void *ret = malloc_unsafe(size);
	return ret;
}

static void pmm_free(void *ptr){
	free_unsafe(ptr);
}
*/
#include "os.h"
#include <unistd.h>
#include <stdint.h>

static void pmm_init();
static void *pmm_alloc(size_t size);
static void pmm_free(void *ptr);

static spinlock_t pmm_lock;
static void *cur = NULL;
static size_t mul = 1;

MOD_DEF(pmm) {
    .init = pmm_init,
    .alloc = pmm_alloc,
    .free = pmm_free
};

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
//	printf("size: %d\t mul: %d\n", size, mul);
	return mul;
}

static Block* allocate_new(size_t size) {
	TRACE_ENTRY;
	void *new_mem;
	Block *new_block;
//	printf("size: 0x%x\n", size);
	if (2 * size + cur > _heap.end)
		return NULL;
	else {
		new_mem = cur;
		cur += 2 * size;
	}
//	Log("new_mem: 0x%x", new_mem);
	new_block = (Block *)new_mem;
	new_block->body.size = 2 * size;
	free_unsafe((void*)(new_block + 1));
	TRACE_EXIT;
	return freep;
}

static void* malloc_unsafe(size_t size) {
	TRACE_ENTRY;
	Block *current, *prev;
	prev = freep;
	size = align(size);
	if (prev == NULL) {
		freep = prev = &base;
		base.body.next = freep;
		base.body.size = 0;
	}
	current = prev->body.next;
	while (1) {
		if (current->body.size >= size) {
//			Log("current->body.size: %d\tsize: %d\n", current->body.size, size);
			if (current->body.size > size) {
				current->body.size -= size;
				current = (Block *)((char *)current + current->body.size);
				current->body.size = size;
				while ((size_t) current % mul != 0) {
					current = (Block *)((char *)current - 1);
					current->body.size ++;
				}
			}
			else {
				prev->body.next = current->body.next;
			}
			freep = prev;
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
//	Log("0x%x mod 0x%x = 0x%x", ret, mul, (size_t)ret % mul);
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

///Linked-list version
#include <stddef.h>
#include <unistd.h>

typedef struct list_t list_t;

struct list_t {
	size_t  size; //size including list_t
	list_t* next; //next available block.
	int free;
	char    data[]; //C99 flexible array.
};

#define LIST_T_SIZE sizeof(list_t)
void *global_head = NULL;

list_t *request_space(list_t* last, size_t size)
{
	list_t* block = sbrk(0);
	void* req = sbrk(size + LIST_T_SIZE);
	if (req == (void*) -1) {
		return NULL;
	}
	if (last) {
		last->next = block;
	}
	block->size = size;
	block->next = NULL;
	block->free = 0;
	return block;	
}

list_t *find_free_block(list_t** last, size_t size) 
{
	list_t* current = global_head;
	while (current && !(current->free && current->size >= size)) {
		*last = current;
		current = current->next;	
	}
	return current;
}

void *malloc(size_t size)
{
	list_t* block;
	if (size <= 0)
		return;
	if (!global_head) {
		block = request_space(NULL, size);
		if (!block) {
			return NULL;
		}
	} else {
		list_t* last = global_head;
		block = find_free_block(&last, size);
		if (!block) {
			block = request_space(last, size);
			if (!block) {
				return NULL;
			}
		} else {
			block->free = 0;
		}
	}
	return (block+1);
}

void *calloc(size_t nitems, size_t size)
{
}

void *realloc(void *ptr, size_t size)
{
}

void free(void *ptr)
{
	if (!ptr) {
		return;
	}
	list_t* block_ptr = (list_t*)ptr-1;
	block_ptr->free = 1;
}
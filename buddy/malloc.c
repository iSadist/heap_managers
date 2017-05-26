//Buddy system version

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <math.h>

#define N (32) //Memory size (index)
#define MIN_SIZE 1; //Minimum block size
#define LIST_T_SIZE sizeof(list_t)
#define MAX_SIZE (1LL << N)

//Data structures
typedef struct list_t list_t;

struct list_t
{
  unsigned free:1;     /* one if free. */
  size_t   size;       /* size including list_t */
  list_t*  succ;       /* successor block in list. */
  list_t*  pred;       /* predecessor block in list. */
  char     data[];
};

list_t* free_list[N];
void* start = NULL;
unsigned int init = 1;


// void print_list()
// {
// 	for (int i = 0 ; i < N ; i++) {
// 		list_t* tmp = free_list[i];
// 		if (tmp) {
// 			fprintf(stderr, "At index %d block with size %zu is free %d\n",i, tmp->size, tmp->free);
// 			if (tmp->succ) {
// 				fprintf(stderr, "Succ index %d block with size %zu is free %d\n",i, tmp->succ->size, tmp->succ->free);
// 			}
// 			if (tmp->pred) {
// 				fprintf(stderr, "Pred index %d block with size %zu is free %d\n",i, tmp->pred->size, tmp->pred->free);
// 			}
// 		}
// 	}
// }

/*Keep a linked list (n) for each possible memory size 2^n == memory size.
All empty except for the largest memory size which has a block*/

void init_blocks()
{
	list_t* block = sbrk(MAX_SIZE);
	if (block == (void*) -1) {
    write(2, "init fail\n", 10);
		return;
	}

	start = block;

  block->size = N;
  block->free = 1;
	block->pred = NULL;
	block->succ = NULL;

  free_list[N-1] = block;
}

size_t rounded_size(size_t size)
{
	size = size + LIST_T_SIZE;
  size_t rounded_size = MIN_SIZE;
  while (rounded_size < MAX_SIZE && rounded_size < size) {
    rounded_size <<= 1;
  }
  return rounded_size;
}

//Recursive method
list_t* recursive_alloc(size_t index, size_t start, size_t size)
{
  if (index > N-1)
  {
    return NULL;
  }

	list_t* avail = free_list[index];

	//Remove from the free_list
  free_list[index] = free_list[index]->succ;

  //If the block has the same size as the expected size
  if (index == start) {
    avail->free = 0;
    if (avail->succ != NULL) {
      avail->succ->pred = NULL;
      avail->succ = NULL;
    }
    return avail;
  }

  if (avail->succ != NULL) {
    avail->succ->pred = NULL;
	}

	//fprintf(stderr, "%s%zu %s %zu\n", "Splitting 2^", index, "for size", size);

	//Split into two blocks
	unsigned int new_size = avail->size - 1;

	list_t* first_half = avail;
	list_t* second_half = (list_t*) ((char*) first_half + (1L << new_size));

	first_half->free = 1;
	first_half->size = new_size;
	first_half->pred = NULL;
	first_half->succ = second_half;

	second_half->free = 1;
	second_half->size = new_size;
	second_half->pred = first_half;
	second_half->succ = NULL;

	//Add the element with half the size to the list
	free_list[index-1] = first_half;

  return recursive_alloc(index-1, start, size);
}

list_t* allocate_memory(size_t index, size_t size)
{
  size_t start_index = index;
  list_t* avail = free_list[index];
	while (avail == NULL && index < N) {
		index++;
	  avail = free_list[index];
	}

  if (!avail) {
    char* mess = "MALLOC IS FAILING DUE TO A PROBLEM!\n";
    write(2, mess, sizeof(mess));
    return NULL;
  }

  return recursive_alloc(index, start_index, size);
}

void *malloc(size_t size)
{
	/*Round size up to power of two = y
	Check if available block of resulting size by taking the log base 2 of y
	In C you can write = log(x)/log(2)
	to get the free list number.
	Otherwise check upwards in sizes, splitting each in two
	If no space return null*/
  if (size <= 0 || size > MAX_SIZE)
    return NULL;

  if (init) {
    init_blocks();
    init = 0;
  }

  size_t r_size = rounded_size(size);
  size_t index = log(r_size)/log(2);
  list_t* block = allocate_memory(index-1, r_size);

	return block->data;
}

void *calloc(size_t nitems, size_t size)
{
	void *memory = malloc(nitems * size);

	if(memory != NULL)
		memset(memory, 0, nitems * size);
	return memory;
}

void *realloc(void *ptr, size_t size)
{
	void *new_memory = malloc(size);

	if (new_memory != NULL) {
		memmove(new_memory, ptr, size);
		free(ptr);
	}
	return new_memory;
}

list_t* recurse_merge(list_t* block_ptr)
{
	if (block_ptr->size == N)
		return block_ptr;

	size_t size = block_ptr->size;
	size_t index = size-1;

  void* buddy = start + (((void*)block_ptr - start) ^ (1L << block_ptr->size));

  list_t* buddy_ptr = (list_t*) buddy;
	list_t* merged_segment = block_ptr;

  if (buddy_ptr->free && (buddy_ptr->size == block_ptr->size)) {
    // fprintf(stderr, "%s %p %p\n", "Free buddy", (void*)block_ptr, (void*)buddy_ptr);
		if (block_ptr < buddy_ptr)
    	merged_segment = block_ptr;
    else
      merged_segment = buddy_ptr;

		if (buddy_ptr == free_list[index]) {
    	free_list[index] = buddy_ptr->succ;
      if (free_list[index] != NULL) {
      	free_list[index]->pred = NULL;
			}
    } else {
    	buddy_ptr->pred->succ = buddy_ptr->succ;
      if (buddy_ptr->succ != NULL) {
        buddy_ptr->succ->pred = buddy_ptr->pred;
      }
    }
		merged_segment->size = size + 1;
		recurse_merge(merged_segment);
  }
	return merged_segment;
}

void free(void *ptr)
{
	/*If buddy also free => merge the blocks and recurse until no free buddy
	add the resulting block to list
	Else add block to list*/

  if (ptr == NULL) {
        return;
  }

	list_t* block_ptr = (list_t*)((char*)ptr - LIST_T_SIZE);

	block_ptr = recurse_merge(block_ptr);

	block_ptr->free = 1;
	size_t size = block_ptr->size;
	size_t index = size-1;

	if (free_list[index] == NULL) {
		free_list[index] = block_ptr;
    free_list[index]->succ = NULL;
    free_list[index]->pred = NULL;
    return;
	}
	list_t* p = free_list[index];
  list_t* prev = NULL;
  while (p != NULL && p < block_ptr) {
    prev = p;
    p = p->succ;
	}
	if (prev == NULL) {
    free_list[index] = block_ptr;
    free_list[index]->succ = p;
    free_list[index]->pred = NULL;
    p->pred = block_ptr;
    return;
  }
  if (p == NULL) {
    prev->succ = block_ptr;
    block_ptr->succ = NULL;
		block_ptr->pred = prev;
    return;
  }
  block_ptr->succ = p;
  block_ptr->pred = prev;
  prev->succ = block_ptr;
	p->pred = block_ptr;
}

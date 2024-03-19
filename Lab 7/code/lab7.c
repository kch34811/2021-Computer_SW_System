/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

#define MAX_POWER 50
#define TAKEN 1
#define FREE 0

#define WSIZE 4 /* bytes */
#define DSIZE 8
#define CHUNK ((1<<12)/WSIZE) /* extend heap by this amount (words) */
#define BIT_SIZE 3 // bits
#define HDR_FTR_SIZE 2 // in words
#define FTR_SIZE 1 // in words
#define PRED_FIELD_SIZE 1 // in words
#define EPILOG_SIZE 2 // in words

// Read and write a word at address p
#define PUT_WORD(p, val) (*(char **)(p) = (val))

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

// Pack a size and allocated bit into a BIT_word
#define PACK(size, status) ((size<<BIT_SIZE) | (status))

// Read the size and allocation bit from address p
#define GET_SIZE(p)  (((*(unsigned int *)(p)) & ~((1 << BIT_SIZE) - 1)) >> BIT_SIZE)
#define GET_STATUS(p) ((*(unsigned int *)(p)) & 0x1)

// Address of block's footer
// Take in a pointer that points to the header
#define FTRP(header_p) ((char **)(header_p) + GET_SIZE(header_p) + 1)

// Get total size of a block
// Size indicates the size of the free space in a block
// Total size = size + size_of_header + size_of_footer = size + DSIZE
// p must point to a header
#define GET_TOTAL_SIZE(p) (GET_SIZE(p) + HDR_FTR_SIZE)

// Define this so later when we move to store the list in heap,
// we can just change this function
#define GET_FREE_LIST_PTR(i) (*(free_lists+i))
#define SET_FREE_LIST_PTR(i, ptr) (*(free_lists+i) = ptr)

// Set pred or succ for free blocks
#define SET_PTR(p, ptr) (*(char **)(p) = (char *)(ptr))

// Get pointer to the word containing the address of pred and succ for a free block
// ptr should point to the start of the header
#define GET_PTR_PRED_FIELD(ptr) ((char **)(ptr) + 1)
#define GET_PTR_SUCC_FIELD(ptr) ((char **)(ptr) + 1 + PRED_FIELD_SIZE)

// Get the pointer that points to the succ of a free block
// ptr should point to the header of the free block
#define GET_PRED(p) (*(GET_PTR_PRED_FIELD(p)))
#define GET_SUCC(p) (*(GET_PTR_SUCC_FIELD(p)))

// Given pointer to current block, return pointer to header of previous block
#define PREV_BLOCK_IN_HEAP(header_p) ((char **)(header_p) - GET_TOTAL_SIZE((char **)(header_p) - FTR_SIZE))

// Given pointer to current block, return pointer to header of next block
#define NEXT_BLOCK_IN_HEAP(header_p) (FTRP(header_p) + FTR_SIZE)

static char **free_lists;
static char **heap_ptr;

static size_t find_free_index (size_t size);
static void *coalesce(void *p);
static void *extend_heap(size_t size);
static void *find_free_block(size_t size);
static void alloc_free_block(void *p, size_t asize);
static void insert_list (char **p);
static void delete_list (char **p);

int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);
int mm_check();

static int round_up (size_t);

static size_t find_free_index (size_t size) {
    
    int index = 0;
    while ((index <= MAX_POWER) && (size > 1))
    {
        size >>= 1;
        index++;
    }
    return index;
}

static void *coalesce(void *p) {
	
    char **prev_alloc = PREV_BLOCK_IN_HEAP(p);
	char **next_alloc = NEXT_BLOCK_IN_HEAP(p);

    size_t prev_status = GET_STATUS(prev_alloc);
	size_t next_status = GET_STATUS(next_alloc);
	size_t size = GET_SIZE(p);

	if (prev_status == FREE && next_status == FREE) {
		delete_list (prev_alloc);
		delete_list (next_alloc);
		size += GET_TOTAL_SIZE(prev_alloc) + GET_TOTAL_SIZE(next_alloc);

		PUT_WORD(prev_alloc, PACK(size, FREE));
		PUT_WORD(FTRP(next_alloc), PACK(size, FREE));
		p = prev_alloc;
	}
    else if (prev_status == FREE && next_status == TAKEN) {
		delete_list (prev_alloc);
		size += GET_TOTAL_SIZE(prev_alloc);

		PUT_WORD(prev_alloc, PACK(size, FREE));
		PUT_WORD(FTRP(p), PACK(size, FREE));
		p = prev_alloc;
	} 
    else if (prev_status == TAKEN && next_status == FREE) {
		delete_list (next_alloc);
		size += GET_TOTAL_SIZE(next_alloc);

		PUT_WORD(p, PACK(size, FREE));
		PUT_WORD(FTRP(next_alloc), PACK(size, FREE));
	} 
    else if (prev_status == TAKEN && next_status == TAKEN) {
		return p;
	} 

	return p;
}

static void *extend_heap(size_t size) {
	char **p; // pointer to free block
	char **p_last; // pointer to the last of free block
	size_t word_extension = (size + 1) & ~1; // make sure double aligned
	size_t all_word_extension = word_extension + HDR_FTR_SIZE; // header + footer

	if ((long)(p = mem_sbrk((all_word_extension) * WSIZE)) == -1) {
		return NULL;
	}

	p -= EPILOG_SIZE;

	PUT_WORD(p, PACK(word_extension, FREE));
	PUT_WORD(FTRP(p), PACK(word_extension, FREE));

	p_last = p + all_word_extension;
	PUT_WORD(p_last, PACK(0, TAKEN));
	PUT_WORD(FTRP(p_last), PACK(0, TAKEN));

	return p;
}

static void *find_free_block(size_t size) {
	char **p;
	size_t index = find_free_index (size);

	// check first free list
	if ((p = GET_FREE_LIST_PTR(index)) != NULL && GET_SIZE(p) >= size) {
		while(1) {
			if (GET_SIZE(p) == size) {
				return p;
			}

			if (!(GET_SUCC(p) == NULL || GET_SIZE(GET_SUCC(p)) < size)) {
                p = GET_SUCC(p);
			} 
            else {
				return p;
			}
		}
	}

	// move to current free list
	index++;

	// non-empty free list
	while (GET_FREE_LIST_PTR(index) == NULL && index < MAX_POWER) {
		index++;
	}

	// if non-NULL free list, go to smallest block
	if ((p = GET_FREE_LIST_PTR(index)) != NULL) {
		while (GET_SUCC(p) != NULL) {
			p = GET_SUCC(p);
		}
		return p;
	} 
    else { 
		return NULL;
	}
}

static void alloc_free_block(void *p, size_t asize) {
	size_t p_size = GET_SIZE(p);
	int block_size = p_size + - asize - HDR_FTR_SIZE;

	char **block;
	if (block_size == 0) {
		asize += HDR_FTR_SIZE;

		PUT_WORD(p, PACK(asize, TAKEN));
		PUT_WORD(FTRP(p), PACK(asize, TAKEN));
	}
    else if ((int)block_size > 0) {

		block = (char **)(p) + asize + HDR_FTR_SIZE;

		PUT_WORD(block, PACK(block_size, FREE));
		PUT_WORD(FTRP(block), PACK(block_size, FREE));

		PUT_WORD(p, PACK(asize, TAKEN));
		PUT_WORD(FTRP(p), PACK(asize, TAKEN));

		block = coalesce(block);

		insert_list (block);
	} 
    else {
		PUT_WORD(p, PACK(asize, TAKEN));
		PUT_WORD(FTRP(p), PACK(asize, TAKEN));
	}
}

static void insert_list (char **p) {
    size_t size = GET_SIZE(p);
    int index = find_free_index (size);

    char **fptr = GET_FREE_LIST_PTR(index);
    char **bptr = NULL;

	if (size == 0) return;

    if (fptr == NULL)
    {
        SET_PTR(GET_PTR_SUCC_FIELD(p), NULL);
		SET_PTR(GET_PTR_PRED_FIELD(p), NULL);
        SET_FREE_LIST_PTR(index, p);
        return;
    }

    if (size >= GET_SIZE(fptr))
    {
		SET_FREE_LIST_PTR(index, p);
        SET_PTR(GET_PTR_SUCC_FIELD(p), fptr);
		SET_PTR(GET_PTR_PRED_FIELD(fptr), p);
		SET_PTR(GET_PTR_PRED_FIELD(p), NULL);
        return;
    }

    while (fptr != NULL && GET_SIZE(fptr) > size)
    {
        bptr = fptr;
        fptr = GET_SUCC(fptr);
    }

    // last free list
    if (fptr == NULL)
    {
        SET_PTR(GET_PTR_SUCC_FIELD(bptr), p);
		SET_PTR(GET_PTR_PRED_FIELD(p), bptr);
        SET_PTR(GET_PTR_SUCC_FIELD(p), NULL);
        return;
    }
    else
    { 
        SET_PTR(GET_PTR_SUCC_FIELD(bptr), p);
				SET_PTR(GET_PTR_PRED_FIELD(p), bptr);
        SET_PTR(GET_PTR_SUCC_FIELD(p), fptr);
				SET_PTR(GET_PTR_PRED_FIELD(fptr), p);
        return;
    }

}

static void delete_list (char **p) {
	char **prev_alloc = GET_PRED(p);
	char **next_alloc = GET_SUCC(p);
	int index;

	if (GET_SIZE(p) == 0) return;

	if (prev_alloc == NULL) {
		index = find_free_index (GET_SIZE(p));
		GET_FREE_LIST_PTR(index) = next_alloc;
	} else { 
		SET_PTR(GET_PTR_SUCC_FIELD(prev_alloc), next_alloc);
	}

	if (next_alloc != NULL) {
		SET_PTR(GET_PTR_PRED_FIELD(next_alloc), prev_alloc);
	}

	SET_PTR(GET_PTR_PRED_FIELD(p), NULL);
	SET_PTR(GET_PTR_SUCC_FIELD(p), NULL);
}

int mm_init(void) {
    // Store free list pointer on heap
	if ((long)(free_lists = mem_sbrk(((MAX_POWER + 1) & ~1) *sizeof(char *))) != -1) {
        
        int i = 0;
        for (i = 0; i <= MAX_POWER; i++) {
	        SET_FREE_LIST_PTR(i, NULL);
        }

	    // align to double word
	    mem_sbrk(WSIZE);

        if ((long)(heap_ptr = mem_sbrk(4*WSIZE)) != -1) {
            PUT_WORD(heap_ptr, PACK(0, TAKEN)); // Prolog header
            PUT_WORD(FTRP(heap_ptr), PACK(0, TAKEN)); // Prolog footer

		    char ** epilog = NEXT_BLOCK_IN_HEAP(heap_ptr);
            PUT_WORD(epilog, PACK(0, TAKEN)); // Epilog header

		    heap_ptr += HDR_FTR_SIZE; // Move past prolog

		    char **new_block;

            if ((new_block = extend_heap(CHUNK)) != NULL){
                insert_list (new_block);
                return 0;
            }
            else return -1;      
        } // 2 for prolog, 2 for epilog
        else  return -1;
    }
	else return -1;
}

void *mm_malloc(size_t size) {
    if (size <= 1<<12) {
		size = round_up(size);
	}

	size_t words = ALIGN(size) / WSIZE;

	size_t size_extension;
	char **p;

	if (size == 0) {
		return NULL;
	}

	// check block is large enough
	// extend the heap
	if ((p = find_free_block(words)) == NULL) {
		if(words <= CHUNK) size_extension = CHUNK;
        else size_extension = words;

		// do not remove block
        if ((p = extend_heap(size_extension)) != NULL) {
			alloc_free_block(p, words);
            return p + 1;
		}
        else return NULL;
	}

	delete_list (p);
	alloc_free_block(p, words);

	return p + 1;
}

void mm_free(void *ptr) {
    ptr -= WSIZE;

    size_t size = GET_SIZE(ptr);

    PUT_WORD(ptr, PACK(size, FREE));
    PUT_WORD(FTRP(ptr), PACK(size, FREE));

    ptr = coalesce(ptr);
    insert_list (ptr);
}

void *mm_realloc(void *ptr, size_t size) {
    static int pre_size;
	int buffer_size;
	int dif = abs(size - pre_size);

	if (!(dif < 1<<12 && dif % round_up(dif))) {
		buffer_size = size % 1000 >= 500 ? size + 1000 - size % 1000 : size - size % 1000; //rounding to thousand
	} else {
		buffer_size = round_up(dif);
	}

    if (ptr == NULL) {
		return mm_malloc(ptr);
	}

	// start of block
	char **old = (char **)ptr - 1;
    char **p = (char **)ptr - 1;

	// get intended and current size
    size_t old_size = GET_SIZE(p); // words
	size_t new_size = ALIGN(size) / WSIZE; // words
	size_t total_buffer = new_size + buffer_size;

	if (total_buffer == old_size && new_size <= total_buffer) {
		return p + 1;
	}

	if (new_size == 0) {
		mm_free(ptr);
		return NULL;
	}
    
    if (new_size > old_size) {
        // checks if possible to merge with both prev and next block in memory
        if (GET_SIZE(PREV_BLOCK_IN_HEAP(p)) + GET_SIZE(NEXT_BLOCK_IN_HEAP(p)) + old_size + 4 >= total_buffer && GET_STATUS(PREV_BLOCK_IN_HEAP(p)) == FREE && GET_STATUS(NEXT_BLOCK_IN_HEAP(p)) == FREE) { 
			PUT_WORD(p, PACK(old_size, FREE));
 	        PUT_WORD(FTRP(p), PACK(old_size, FREE));

 			p = coalesce(p);
			memmove(p + 1, old + 1, old_size * WSIZE);
			alloc_free_block(p, total_buffer);
		} 
        // checks if possible to merge with next block in memory
        else if (GET_SIZE(PREV_BLOCK_IN_HEAP(p)) + old_size + 2 >= total_buffer && GET_STATUS(PREV_BLOCK_IN_HEAP(p)) == FREE && GET_STATUS(NEXT_BLOCK_IN_HEAP(p)) == TAKEN) { 
		    PUT_WORD(p, PACK(old_size, FREE));
 	        PUT_WORD(FTRP(p), PACK(old_size, FREE));

 			p = coalesce(p);

			memmove(p + 1, old + 1, old_size * WSIZE);
 			alloc_free_block(p, total_buffer);
		} 
        // checks if possible to merge with previous block in memory
		else if (GET_SIZE(NEXT_BLOCK_IN_HEAP(p)) + old_size + 2 >= total_buffer && GET_STATUS(PREV_BLOCK_IN_HEAP(p)) == TAKEN && GET_STATUS(NEXT_BLOCK_IN_HEAP(p)) == FREE) { 
			PUT_WORD(p, PACK(old_size, FREE));
	        PUT_WORD(FTRP(p), PACK(old_size, FREE));

			p = coalesce(p);
			alloc_free_block(p, total_buffer);
		} 
        else { // end case: if no optimization possible, just do brute force realloc
			p = (char **)mm_malloc(total_buffer*WSIZE + WSIZE) - 1;

			if (p == NULL) {
				return NULL;
			}

			memcpy(p + 1, old + 1, old_size * WSIZE);
			mm_free(old + 1);
		}
	}
    
	pre_size = size;
	return p + 1;
}

static int round_up (size_t x)
{
    int rvalue = 0;
    if (x >= 0) {
        --x;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        rvalue = x + 1;
    }
    return rvalue;   
}

static void check_free_blocks_marked_free() {
	char ** p;

	int i;
    for (i=0; i <= MAX_POWER; i++) {
		if (p = GET_FREE_LIST_PTR(i)) {
			while (p) {
		        if (GET_STATUS(p) == TAKEN) {
			        printf("There are free blocks that are marked as taken");
			        assert(0);
		        }
		    p = GET_SUCC(p);
	        }
		}
	}

	printf("check_free_blocks_marked_free passed.\n");
}

static void check_contiguous_free_block_coalesced() {
	char ** p = heap_ptr;

	while (GET_STATUS(p) != TAKEN && GET_SIZE(p) != 0) { 

        char ** next_block = NEXT_BLOCK_IN_HEAP(p);

	    if (GET_STATUS(p) == FREE && GET_STATUS(next_block) == FREE) {
		printf("Block %p should coalesce with block %p", p, next_block);
		assert(0);
	    }

		p = NEXT_BLOCK_IN_HEAP(p);
	}

	printf("check_contiguous_free_block_coalesced passed.\n");
}

static void check_all_free_blocks_in_free_list() {
	char ** p = heap_ptr;

	while (GET_STATUS(p) != TAKEN && GET_SIZE(p) != 0) { // Haven't hit epilog
		if (GET_STATUS(p) == FREE) {
			int size = GET_SIZE(p);
            int index = find_free_index (size);

            if (GET_FREE_LIST_PTR(index) == p)
                return; // beginning of free list

            char **prev_block = GET_PRED(p);
            char **next_block = GET_SUCC(p);

            if (!prev_block && !next_block) {
                printf("Free block %p not in free list", p);
                assert(0);
            }			
		}
		p = NEXT_BLOCK_IN_HEAP(p);
	}

	printf("check_all_free_blocks_in_free_list passed.\n");
}

static void check_all_free_blocks_valid() {
	char ** p = heap_ptr;
    size_t size_in_hdr;
    size_t size_in_ftr;

	while (GET_STATUS(p) != TAKEN && GET_SIZE(p) != 0) { 
		if (GET_STATUS(p) == FREE) {
			// is valid free block
            size_in_hdr = GET_SIZE(p);
            size_in_ftr = GET_SIZE(FTRP(p));

            // size in hearder == size in footer
            if (size_in_hdr != size_in_ftr) {
                printf("Free block %p has different sizes in hdr and ftr", p);
                assert(0);
            }

            // status in header and footer
            if (GET_STATUS(p) == TAKEN) {
                printf("Free block %p has status as taken in header", p);
                assert(0);
            }

            if (GET_STATUS(FTRP(p)) == TAKEN) {
                printf("Free block %p has status as taken in footer", p);
                assert(0);
            }
		}
		p = NEXT_BLOCK_IN_HEAP(p);
	}

	printf("check_all_free_blocks_valid_ftr_hdr passed.\n");
}

static void check_ptrs_valid_heap_address() {
	void * heap_lo = mem_heap_lo();
	void * heap_hi = mem_heap_hi();

	char ** p = heap_ptr;

	do {
        if (!(heap_lo <= p <= heap_hi)) {
		    printf("%x not in heap range", p);
		    assert(0);
	    }
		p = NEXT_BLOCK_IN_HEAP(p);
	} while (GET_STATUS(p) != TAKEN && GET_SIZE(p) != 0); // Haven't hit epilog

	printf("check_ptrs_valid_heap_address passed.\n");

}

int mm_check() {
	printf("RUNNING MM CHECK.\n");
	check_free_blocks_marked_free();
	check_contiguous_free_block_coalesced();
	check_all_free_blocks_in_free_list();
	check_all_free_blocks_valid();
	check_ptrs_valid_heap_address();
	
	printf("MM CHECK FINISHED SUCCESSFUL.\n\n");
}
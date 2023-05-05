/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  Blocks are never coalesced or reused.  The size of
 * a block is found at the first aligned word before the block (we need
 * it for realloc).
 *
 * This code is correct and blazingly fast, but very bad usage-wise since
 * it never frees anything.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define SIZE_PTR(p)  ((size_t*)(((char*)(p)) - SIZE_T_SIZE))

typedef int Bool;
#define True 1
#define False 0

// size_t : 8 bytes
// align : 8 bytes 

#define CHUNKSIZE (1 << 12)
#define LEAD_AREA_SIZE (1 << 4) // 16 bytes for lead area

#define GET(p) (*((size_t*)(p)))             // get a size_t type v from addr p
#define PUT(p, v) (*((size_t*)(p)) = v)      // put a size_t type v at addr p
#define PACK(size, alloc) ((size) | (alloc)) // pack size_ and allocated_judger_
#define GET_SIZE(p) (GET(p) & (~0x7))        // get size_
#define GET_ALLOC(p) (GET(p) & 0x1)          // get allocated_judger_

static char* heap_start_ptr_ = NULL; // point to the first byte of the heap
static char* heap_end_ptr_ = NULL; // point to the last byte of the heap
static char* first_block_ptr_ = NULL;
static char* last_block_ptr_ = NULL;

// a "ptr" points to the head of the block, a "addr" points to the head of its payload
char* get_pre_block_ptr(char* _cur_ptr){
  if(_cur_ptr == first_block_ptr_) return NULL;
  return _cur_ptr - GET_SIZE(_cur_ptr + ALIGNMENT) - 2 * ALIGNMENT;
}

char* get_next_block_ptr(char* _cur_ptr){
  if(_cur_ptr == last_block_ptr_) return NULL;
  return _cur_ptr + 2 * ALIGNMENT + GET_SIZE(_cur_ptr);
}

size_t get_block_size(char* _cur_ptr){
  return GET_SIZE(_cur_ptr);
}

Bool get_block_alloc(char* _cur_ptr){
  return GET_ALLOC(_cur_ptr);
}

void update_block_alloc(char* _cur_ptr){
  PUT(_cur_ptr, GET(_cur_ptr) ^ 0x1);
}

// imexplicit free list 
// the struct of a block : [8 bytes for (size_ of self, allocated_judger_)] <-- _cur_ptr
//                         [8 bytes for (size_ of pre, allocated_judger_)]
//                         [payload]                                        <-- addr
// size_ means the size of a block's payload

// segregated lists 
// 8 * {1}, {2}, {3-4}, {5-8}, {9-16}, ..., {(2^{n-1} + 1)-(2^{n})}, ...

/*
 * mm_init - Called when a new trace starts.
 */
int mm_init(void){
  // create initial space
  void* ck_ptr = mem_sbrk(CHUNKSIZE);
  // check if successful
  if(ck_ptr == NULL) return -1;
  heap_start_ptr_ = mem_heap_lo();
  heap_end_ptr_ = mem_heap_hi();
  // allocate a lead field
  first_block_ptr_ = heap_start_ptr_ + LEAD_AREA_SIZE;
  last_block_ptr_ = heap_start_ptr_ + LEAD_AREA_SIZE;
  // init block
  size_t init_block_size_ = CHUNKSIZE - LEAD_AREA_SIZE - 2 * ALIGNMENT;
  PUT(first_block_ptr_, PACK(init_block_size_, False));
  PUT(first_block_ptr_ + ALIGNMENT, PACK(0, False));
  return 0;
}

/*
 * malloc - Allocate a block by incrementing the brk pointer.
 *      Always allocate a block whose size is a multiple of the alignment.
 */
void *malloc(size_t size){ 
  if(size <= 0) return NULL;
  // now size > 0
  size_t actual_size = ALIGN(size);
  // find a fit one
  char* tmp_ptr = NULL;
  for(tmp_ptr = first_block_ptr_; tmp_ptr != NULL; tmp_ptr = get_next_block_ptr(tmp_ptr)){
    if(get_block_alloc(tmp_ptr) == False){
      size_t tmp_block_size = get_block_size(tmp_ptr);
      if(tmp_block_size >= actual_size + 3 * ALIGNMENT){ 
        // split the block and take the first one
        // [s.s.] [p.s.] [pl] | [n.s.] [s.s.] [pl] --> [s.ns.] [p.s.] [[ns.pl] [ns.] []]
        char* next_ptr = get_next_block_ptr(tmp_ptr);
        PUT(tmp_ptr, PACK(actual_size, True));
        char* new_block_ptr = tmp_ptr + 2 * ALIGNMENT + actual_size; // now the next must change
        PUT(new_block_ptr, PACK(tmp_block_size - actual_size - 2 * ALIGNMENT, False));
        PUT(new_block_ptr + ALIGNMENT, PACK(actual_size, True));
        if(next_ptr == NULL){
          // new last block
          last_block_ptr_ = new_block_ptr;
        }
        else{
          // update in next block
          PUT(next_ptr + ALIGNMENT, PACK(tmp_block_size - actual_size - 2 * ALIGNMENT, False));
        }
        return tmp_ptr + 2 * ALIGNMENT;
      }
      else if(tmp_block_size >= actual_size){
        // just take the block
        update_block_alloc(tmp_ptr);
        // update in next block
        char* next_ptr = get_next_block_ptr(tmp_ptr);
        if(next_ptr != NULL) update_block_alloc(next_ptr + ALIGNMENT);
        return tmp_ptr + 2 * ALIGNMENT;
      }
    }
  }
  // miss
  return NULL;
}

/*
 * free - We don't know how to free a block.  So we ignore this call.
 *      Computers have big memories; surely it won't be a problem.
 */
void free(void *ptr){
	/*Get gcc to be quiet */
	if(ptr == NULL) return;
  char* cur_block_ptr = ptr - 2 * ALIGNMENT;
  char* pre_block_ptr = get_pre_block_ptr(cur_block_ptr);
  char* next_block_ptr = get_next_block_ptr(cur_block_ptr);
  if(pre_block_ptr != NULL && get_block_alloc(pre_block_ptr) == False){
    if(next_block_ptr != NULL && get_block_alloc(next_block_ptr) == False){
      // merge both pre and next 
      size_t new_size = get_block_size(pre_block_ptr) + 2 * ALIGNMENT + get_block_size(cur_block_ptr) + 2 * ALIGNMENT + get_block_size(next_block_ptr);
      PUT(pre_block_ptr, PACK(new_size, False));
      if(next_block_ptr == last_block_ptr_){
        // new last block
        last_block_ptr_ = pre_block_ptr;
      }
      else{
        // update in next_next block
        char* nn_block_ptr = get_next_block_ptr(next_block_ptr);
        PUT(nn_block_ptr + ALIGNMENT, PACK(new_size, False));
      }
    }
    else{
      // merge with pre 
      size_t new_size = get_block_size(pre_block_ptr) + 2 * ALIGNMENT + get_block_size(cur_block_ptr);
      PUT(pre_block_ptr, PACK(new_size, False));
      if(next_block_ptr == NULL){
        // new last block
        last_block_ptr_ = pre_block_ptr;
      }
      else{
        // update in next block
        PUT(next_block_ptr + ALIGNMENT, PACK(new_size, False));
      }
    }
  }
  else{
    if(next_block_ptr != NULL && get_block_alloc(next_block_ptr) == False){
      // merge with next 
      // [8] [8] [pl] | [8] [8] [pl] | [8] [8] ---> [8] [8!] [pl!] | [8] [8!]
      size_t new_size = get_block_size(cur_block_ptr) + 2 * ALIGNMENT + get_block_size(next_block_ptr);
      PUT(cur_block_ptr, PACK(new_size, False));
      if(next_block_ptr == last_block_ptr_){
        // new last block
        last_block_ptr_ = cur_block_ptr;
      }
      else{
        // update in next_next block
        char* nn_block_ptr = get_next_block_ptr(next_block_ptr);
        PUT(nn_block_ptr + ALIGNMENT, PACK(new_size, False));
      }
    }
    else{
      // only occupy cur and update next 
      update_block_alloc(cur_block_ptr);
      if(next_block_ptr != NULL) update_block_alloc(next_block_ptr + ALIGNMENT);
    }
  }
}

/*
 * realloc - Change the size of the block by mallocing a new block,
 *      copying its data, and freeing the old block.  I'm too lazy
 *      to do better.
 */
void *realloc(void *oldptr, size_t size){
  if(oldptr == NULL){
    return malloc(size);
  }
  // now oldptr != NULL 
  if(size <= 0){
    free(oldptr);
    return NULL;
  }
  // now size > 0 
  size_t actual_size = ALIGN(size);
  char* cur_block_ptr = oldptr - 2 * ALIGNMENT;
  size_t cur_block_size = get_block_size(cur_block_ptr);
  if(cur_block_size >= actual_size + 3 * ALIGNMENT){
    // split the block and try to merge next of new block
    size_t new_block_size = cur_block_size - actual_size - 2 * ALIGNMENT;
    char* next_block_ptr = get_next_block_ptr(cur_block_ptr);
    PUT(cur_block_ptr, PACK(actual_size, True));
    char* new_block_ptr = cur_block_ptr + 2 * ALIGNMENT + actual_size;
    PUT(new_block_ptr + ALIGNMENT, PACK(actual_size, True)); // this is always the same
    if(next_block_ptr == NULL){
      PUT(new_block_ptr, PACK(new_block_size, False)); 
      // new last block
      last_block_ptr_ = new_block_ptr;
    }
    else{
      if(get_block_alloc(next_block_ptr) == True){
        PUT(new_block_ptr, PACK(new_block_size, False)); 
        // update in next block
        PUT(next_block_ptr + ALIGNMENT, PACK(new_block_size, False));
      }
      else{
        char* nn_block_ptr = get_next_block_ptr(next_block_ptr);
        size_t next_block_size = get_block_size(next_block_ptr);
        PUT(new_block_ptr, PACK(new_block_size + next_block_size, False));
        if(nn_block_ptr == NULL){
          // new last block
          last_block_ptr_ = new_block_ptr;
        }
        else{
          // update in next block
          PUT(nn_block_ptr + ALIGNMENT, PACK(new_block_size + next_block_size, False));
        }
      }
    }
    return cur_block_ptr + 2 * ALIGNMENT;
  }
  else if(cur_block_size >= actual_size){
    // just return old ptr
    return oldptr;
  }
  else{
    // we need more space !
    char* next_block_ptr = get_next_block_ptr(cur_block_ptr);
    if(next_block_ptr != NULL && get_block_alloc(next_block_ptr) == False){ // nn must True!
      size_t next_block_size = get_block_size(next_block_ptr);
      if(cur_block_size + 2 * ALIGNMENT + next_block_size >= actual_size + 3 * ALIGNMENT){
        // merge with next and split 
        // [8] [8] [cr_siz] | [8] [8] [nx_siz] ---> [8] [8] [ac_siz] | [8] [8] [nw_siz]
        char* nn_block_ptr = get_next_block_ptr(next_block_ptr);
        size_t new_block_size = cur_block_size + next_block_size - actual_size;
        char* new_block_ptr = cur_block_ptr + 2 * ALIGNMENT + actual_size;
        PUT(cur_block_ptr, PACK(actual_size, True));
        PUT(new_block_ptr, PACK(new_block_size, False));
        PUT(new_block_ptr + ALIGNMENT, PACK(actual_size, True));
        if(nn_block_ptr == NULL){
          // new last block
          last_block_ptr_ = new_block_ptr;
        }
        else{
          // update in nn block
          PUT(nn_block_ptr + ALIGNMENT, PACK(new_block_size, False));
        }
        return cur_block_ptr + 2 * ALIGNMENT;
      }
      else if(cur_block_size + 2 * ALIGNMENT + next_block_size >= actual_size){
        // only merge 
        // [8] [8] [cr_siz] | [8] [8] [nx_siz] ---> [8] [8] [nw_siz]
        char* nn_block_ptr = get_next_block_ptr(next_block_ptr);
        size_t new_block_size = cur_block_size + 2 * ALIGNMENT + next_block_size;
        PUT(cur_block_ptr, PACK(new_block_size, True));
        if(nn_block_ptr == NULL){
          // new last block
          last_block_ptr_ = cur_block_ptr;
        }
        else{
          // update in nn block
          PUT(nn_block_ptr + ALIGNMENT, PACK(new_block_size, True));
        }
        return cur_block_ptr + 2 * ALIGNMENT;
      }
    }
    // miss next, we need copy 
    char* new_block_ptr = malloc(actual_size); // without ALIGNMENT
    memcpy(new_block_ptr, cur_block_ptr + 2 * ALIGNMENT, cur_block_size);
    free(cur_block_ptr + 2 * ALIGNMENT);
    return new_block_ptr;
  }
}

/*
 * calloc - Allocate the block and set it to zero.
 */
void *calloc (size_t nmemb, size_t size){
  if(nmemb <= 0 || size <= 0) return NULL;
  size_t actual_size = ALIGN(nmemb * size);
  char* ret_ptr = malloc(actual_size);
  memset(ret_ptr, 0, actual_size);
  return ret_ptr;
}

/*
 * mm_checkheap - There are no bugs in my code, so I don't need to check,
 *      so nah!
 */
void mm_checkheap(int verbose){
	/*Get gcc to be quiet. */
	verbose = verbose;
}


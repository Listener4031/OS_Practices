#ifndef OS_MM_H
#define OS_MM_H
#define MAX_ERRNO 4095

#define OK          0
#define EINVAL      22  /* Invalid argument */    
#define ENOSPC      28  /* No page left */  

typedef int Bool;
#define True 1
#define False 0

typedef struct S1{
    struct S1* rank_pre_;
    struct S1* rank_next_;
    struct S1* addr_pre_;
    struct S1* addr_next_;
    int rank_;
    void* start_addr_;
    Bool occupied_judger_;
} PageUnit;

typedef struct S2{
    int rank_;
    PageUnit* head_;
    PageUnit* tail_;
    int cnt_page_unit_;
    int cnt_valid_page_unit_;
} ArrayUnit;

typedef struct S3{
    PageUnit* head_;
    PageUnit* tail_;
    int cnt_page_unit_;
    int cnt_occupied_unit_;
} AddrToPageUnit;

typedef struct S4{
    int rank_;
    PageUnit* to_page_unit_;
} Pair;

#define MINUNIT 4 * 1024

#define IS_ERR_VALUE(x) ((x) >= (unsigned long)-MAX_ERRNO)
static inline void *ERR_PTR(long error) { return (void *)error; }
static inline long PTR_ERR(const void *ptr) { return (long)ptr; }
static inline long IS_ERR(const void *ptr) { return IS_ERR_VALUE((unsigned long)ptr); }

__attribute((constructor)) void Init(void);

void print_list(int);

void check_list(int);

int init_page(void *p, int pgcount);
void *alloc_pages(int rank);
int return_pages(void *p);
int query_ranks(void *p);
int query_page_counts(int rank);

#endif
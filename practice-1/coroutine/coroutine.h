/* YOUR CODE HERE */
#ifndef COROUTINE_H
#define COROUTINE_H

#include <setjmp.h>
#include <stdint.h>
#include <pthread.h>

typedef long long cid_t;
#define STACK_SIZE (1 << 17)

typedef int Bool;
#define True 1
#define False 0

typedef int Status;
#define UNAUTHORIZED -1
#define FINISHED 2
#define RUNNING 1
#define WAITING 0

#define PTHREAD_ARRAY_SIZE 100
#define CORO_ARRAY_SIZE 1000

typedef struct SS1{
    cid_t coro_id_;
    struct SS1* far_coro_ptr_;
    Bool ret_int_judger_;
    Status coro_status_;
    int (*coro_fun_ptr_)(void);
    int return_val_;
    jmp_buf context_;
    uint8_t coro_stack_[STACK_SIZE];
} coroutine;

typedef struct SS2{
    pthread_t pid_;
    long long cur_coro_num_;
    coroutine* cur_coro_ptr_;
    coroutine** coro_ptr_array_;
    coroutine* root_coro_ptr_;
    cid_t jmp_judger_;
    Bool new_coro_judger_;
} mypthread;

int get_index(pthread_t);

int co_start(int (*routine)(void));

int co_getid(void);

int co_getret(int cid);

int co_yield(void);

int co_waitall(void);

int co_wait(int cid);

int co_status(int cid);

void check_status(void);

void check_cur_ptr(int);

#endif
/* YOUR CODE HERE */
#ifndef COROUTINE_H
#define COROUTINE_H

#include <setjmp.h>
#include <stdint.h>

typedef long long cid_t;
#define MAXN 50000
#define STACK_SIZE (1 << 18)

typedef int Bool;
#define True 1
#define False 0

typedef int Status;
#define UNAUTHORIZED -1
#define FINISHED 2
#define RUNNING 1
#define WAITING 0

typedef struct SS{
    cid_t coro_id_;
    struct SS* far_coro_ptr_;
    Bool ret_int_judger_;
    Status coro_status_;
    int (*coro_fun_ptr_)(void);
    int return_val_;
    jmp_buf context_;
    uint8_t coro_stack_[STACK_SIZE];
} coroutine;

void debug1(void);

__attribute((constructor)) void init(void);

int co_start(int (*routine)(void));

int co_getid(void);

int co_getret(int cid);

int co_yield(void);

int co_waitall(void);

int co_wait(int cid);

int co_status(int cid);

#endif
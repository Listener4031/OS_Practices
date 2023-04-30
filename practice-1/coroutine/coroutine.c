/* YOUR CODE HERE */

#include "coroutine.h"
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>

static long long cur_coro_num_ = -1; // 1 - base to 0 - base
static coroutine* cur_coro_ptr_ = NULL; // point to cur_coro
static coroutine** coro_ptr_array_ = NULL;
static coroutine* root_coro_ptr_ = NULL;

static cid_t jmp_judger_ = -1;

__attribute((constructor)) void init(){ // before main
    root_coro_ptr_ = (coroutine*) malloc(sizeof(coroutine));
    root_coro_ptr_ -> coro_id_ = -1;
    root_coro_ptr_ -> far_coro_ptr_ = NULL;
    root_coro_ptr_ -> ret_int_judger_ = True;
    root_coro_ptr_ -> coro_status_ = RUNNING; // main keep running
    root_coro_ptr_ -> coro_fun_ptr_ = NULL;
    cur_coro_ptr_ = root_coro_ptr_;
    coro_ptr_array_ = (coroutine**) malloc(1000 * sizeof(coroutine*));
    //printf("debug: finish init %lld.\n", cur_coro_ptr_ -> coro_id_);
}

int co_start(int (*routine)(void)){
    // create new coro
    coroutine* new_coro_ptr_ = (coroutine*) malloc(sizeof(coroutine));
    new_coro_ptr_ -> coro_id_ = ++cur_coro_num_;
    new_coro_ptr_ -> far_coro_ptr_ = cur_coro_ptr_;
    new_coro_ptr_ -> ret_int_judger_ = True;
    new_coro_ptr_ -> coro_status_ = WAITING; // fake
    new_coro_ptr_ -> coro_fun_ptr_ = routine;
    coro_ptr_array_[cur_coro_num_] = new_coro_ptr_;
    // set juder and yield
    new_coro_ptr_ -> coro_status_ = RUNNING;
    jmp_judger_ = cur_coro_num_; // specific jump
    //printf("debug: co_start before yield -- %lld start.\n", new_coro_ptr_ -> coro_id_);
    co_yield();
    //printf("debug: co_start after yield -- %lld exit.\n", new_coro_ptr_ -> coro_id_);
    return new_coro_ptr_ -> coro_id_;
}

int co_getid(){
    if(cur_coro_ptr_ == NULL) exit(-1);
    return cur_coro_ptr_ -> coro_id_;
}

int co_getret(int cid){
    if(cid > cur_coro_num_) exit(-1);
    if(coro_ptr_array_[cid] == NULL) exit(-1);
    return coro_ptr_array_[cid] -> return_val_;
}

int co_yield(){
    if(cur_coro_ptr_ == NULL) exit(-1);
    // differ by judger
    if(jmp_judger_ == -1){ // active use co_yield, choose another running coro to yield
        //printf("debug: co_yield1 before setjmp %lld\n", cur_coro_ptr_ -> coro_id_);
        int t = setjmp(cur_coro_ptr_ -> context_);
        //printf("debug: co_yield1 after setjmp %lld\n", cur_coro_ptr_ -> coro_id_);
        if(t == 0){
            //printf("debug: judger == -1, t == 0\n");
            //exit(-2);
            // select one coro to yield to
            coroutine* targeted_ptr = NULL;
            int i;
            for(i = 0; i <= cur_coro_num_; ++i){
                if((coro_ptr_array_[i] -> coro_id_ != cur_coro_ptr_ -> coro_id_) && (coro_ptr_array_[i] -> coro_status_ == RUNNING)){
                    targeted_ptr = coro_ptr_array_[i];
                    break;
                }
            }
            if(targeted_ptr == NULL){ // except main is running
                coroutine* tmp_ptr = NULL;
                for(tmp_ptr = cur_coro_ptr_ -> far_coro_ptr_; tmp_ptr != NULL; tmp_ptr = tmp_ptr -> far_coro_ptr_){
                    if(tmp_ptr -> coro_status_ == RUNNING){
                        targeted_ptr = tmp_ptr;
                        break;
                    }
                }
            }
            cur_coro_ptr_ = targeted_ptr;
            //printf("debug: in co_yield, to %lld\n", cur_coro_ptr_ -> coro_id_);
            longjmp(cur_coro_ptr_ -> context_, 2);
        }
        else{
            //printf("debug: judger == -1, t != 0\n");
            //exit(-3);
            return 0;
        }
    }
    else{ 
        //printf("debug: co_yield2 before setjmp %lld\n", cur_coro_ptr_ -> coro_id_);
        int t = setjmp(cur_coro_ptr_ -> context_);
        //printf("debug: co_yield2 after setjmp %lld\n", cur_coro_ptr_ -> coro_id_);
        if(t == 0){ // yield by co_start
            // switch cur_coro
            cur_coro_ptr_ = coro_ptr_array_[cur_coro_num_];
            // inline asm and execute function
            jmp_judger_ = -1;
            __asm__ __volatile__(
                "mov sp,%0\n\t"
                :
                :"r"(cur_coro_ptr_ -> coro_stack_ + STACK_SIZE * 1)
                :
            );
            cur_coro_ptr_ -> return_val_ = (*(cur_coro_ptr_ -> coro_fun_ptr_))();
            cur_coro_ptr_ -> coro_status_ = FINISHED;
            // roll_back cur_coro
            co_yield();
        }
        else{ // example : co_yield coro
            //printf("debug: judger != -1, t != 0\n");
            //exit(-5);
            return 0;
        }
    }
}

int co_waitall(){
    /*
    int i;
    for(i = 0; i <= cur_coro_num_; ++i){
        if(coro_ptr_array_[i] -> coro_status_ == RUNNING){
            coro_ptr_array_[i] -> return_val_ = (*(coro_ptr_array_[i] -> coro_fun_ptr_))();
            coro_ptr_array_[i] -> coro_status_ = FINISHED;
        }
    }
    */
    return 0;
}

int co_wait(int cid){
    if(cid > cur_coro_num_) exit(-1);
    if(coro_ptr_array_[cid] == NULL) exit(-1);
    /*
    if(coro_ptr_array_[cid] -> coro_status_ == RUNNING){
        coro_ptr_array_[cid] -> return_val_ = (*(coro_ptr_array_[cid] -> coro_fun_ptr_))();
        coro_ptr_array_[cid] -> coro_status_ = FINISHED;
    }
    */
    return 0;
}

int co_status(int cid){
    if(cid > cur_coro_num_) return UNAUTHORIZED;
    if(coro_ptr_array_[cid] == NULL) return UNAUTHORIZED;
    //if(coro_ptr_array_[cid] -> far_coro_ptr_ != cur_coro_ptr_) return UNAUTHORIZED;
    coroutine* tmp_ptr = NULL;
    for(tmp_ptr = coro_ptr_array_[cid]; tmp_ptr != NULL; tmp_ptr = tmp_ptr -> far_coro_ptr_){
        if(tmp_ptr -> far_coro_ptr_ == cur_coro_ptr_) return coro_ptr_array_[cid] -> coro_status_;
    }
    return UNAUTHORIZED;
}

void debug1(){
    int i;
    for(i = 0; i <= cur_coro_num_; i++){
        printf("check status: coro %d, %d\n", i, coro_ptr_array_[i] -> coro_status_);
    }
}

/*
int co_start(int (*routine)(void));
int co_getid();
int co_getret(int cid);
int co_yield();
int co_waitall();
int co_wait(int cid);
int co_status(int cid);
*/
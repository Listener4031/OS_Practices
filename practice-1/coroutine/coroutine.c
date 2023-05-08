/* YOUR CODE HERE */

#include "coroutine.h"
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>

static mypthread** pthread_ptr_array_ = NULL;
static pthread_mutex_t mutex_;
static int cnt_pthread_; // init : 0, count how many pthreads
//static int targeted_ind_in_yield;

int get_index(pthread_t _pid){
    pthread_mutex_lock(&mutex_);
    int i;
    for(i = 1; i <= cnt_pthread_; ++i){
        if(pthread_ptr_array_[i] -> pid_ == _pid) {
            pthread_mutex_unlock(&mutex_);
            return i;
        }
    }
    pthread_mutex_unlock(&mutex_);
    return -1;
}

__attribute((constructor)) void init(){ // before main
    // init the array
    pthread_ptr_array_ = (mypthread**) malloc(PTHREAD_ARRAY_SIZE * sizeof(mypthread*));
    cnt_pthread_ = 0;
    // initialize mutex 
    pthread_mutex_init(&mutex_, NULL);
}

int add_pthread(pthread_t _pid){
    // update cnt and add in cnt 
    ++cnt_pthread_;
    pthread_ptr_array_[cnt_pthread_] = (mypthread*) malloc(sizeof(mypthread));
    pthread_ptr_array_[cnt_pthread_] -> pid_ = _pid;
    pthread_ptr_array_[cnt_pthread_] -> cur_coro_num_ = -1;
    //pthread_ptr_array_[cnt_pthread_] -> cur_coro_ptr_ = NULL; ? need not?
    //pthread_ptr_array_[cnt_pthread_] -> coro_ptr_array_ = NULL;
    //pthread_ptr_array_[cnt_pthread_] -> root_coro_ptr_ = NULL;
    pthread_ptr_array_[cnt_pthread_] -> jmp_judger_ = -1;
    pthread_ptr_array_[cnt_pthread_] -> new_coro_judger_ = False;
    // add root coro
    pthread_ptr_array_[cnt_pthread_] -> root_coro_ptr_ = (coroutine*) malloc(sizeof(coroutine));
    pthread_ptr_array_[cnt_pthread_] -> root_coro_ptr_ -> coro_id_ = -1;
    pthread_ptr_array_[cnt_pthread_] -> root_coro_ptr_ -> far_coro_ptr_ = NULL;
    pthread_ptr_array_[cnt_pthread_] -> root_coro_ptr_ -> ret_int_judger_ = True;
    pthread_ptr_array_[cnt_pthread_] -> root_coro_ptr_ -> coro_status_ = RUNNING; // main keep running
    pthread_ptr_array_[cnt_pthread_] -> root_coro_ptr_ -> coro_fun_ptr_ = NULL;
    pthread_ptr_array_[cnt_pthread_] -> cur_coro_ptr_ = pthread_ptr_array_[cnt_pthread_] -> root_coro_ptr_;
    pthread_ptr_array_[cnt_pthread_] -> coro_ptr_array_ = (coroutine**) malloc(CORO_ARRAY_SIZE * sizeof(coroutine*));
    return cnt_pthread_;
}

int co_start(int (*routine)(void)){
    // create new coro
    pthread_t targeted_pid = pthread_self();
    int targeted_ind = get_index(targeted_pid);
    if(targeted_ind == -1){
        // add new thread
        pthread_mutex_lock(&mutex_);
        targeted_ind = add_pthread(targeted_pid);
        pthread_mutex_unlock(&mutex_);
        // printf("  debug: create new pthread: %d\n", targeted_ind);
        assert(pthread_ptr_array_[targeted_ind] -> cur_coro_ptr_ == pthread_ptr_array_[targeted_ind] -> root_coro_ptr_);
    }
    pthread_mutex_lock(&mutex_);
    coroutine* new_coro_ptr_ = (coroutine*) malloc(sizeof(coroutine));
    new_coro_ptr_ -> coro_id_ = ++(pthread_ptr_array_[targeted_ind] -> cur_coro_num_);
    new_coro_ptr_ -> far_coro_ptr_ = pthread_ptr_array_[targeted_ind] -> cur_coro_ptr_;
    new_coro_ptr_ -> ret_int_judger_ = True;
    new_coro_ptr_ -> return_val_ = 123;
    new_coro_ptr_ -> coro_status_ = WAITING; // fake
    new_coro_ptr_ -> coro_fun_ptr_ = routine;
    pthread_ptr_array_[targeted_ind] -> coro_ptr_array_[pthread_ptr_array_[targeted_ind] -> cur_coro_num_] = new_coro_ptr_;
    // set juder and yield
    new_coro_ptr_ -> coro_status_ = RUNNING;
    pthread_ptr_array_[targeted_ind] -> jmp_judger_ = pthread_ptr_array_[targeted_ind] -> cur_coro_num_; // specific jump
    pthread_ptr_array_[targeted_ind] -> new_coro_judger_ = True;
    // printf("  debug: thread %d create new coro : %lld\n", targeted_ind, new_coro_ptr_ -> coro_id_);
    pthread_mutex_unlock(&mutex_);
    // yield
    co_yield();
    // printf("  debug: thread %d create coro %lld end.\n", targeted_ind, new_coro_ptr_ -> coro_id_);
    return new_coro_ptr_ -> coro_id_;
}

int co_getid(){
    int targeted_ind = get_index(pthread_self());
    assert(targeted_ind != -1);
    assert(pthread_ptr_array_[targeted_ind] -> cur_coro_ptr_ != NULL);
    return pthread_ptr_array_[targeted_ind] -> cur_coro_ptr_ -> coro_id_;
}

int co_getret(int cid){
    int targeted_ind = get_index(pthread_self());
    assert(targeted_ind != -1);
    assert(cid <= pthread_ptr_array_[targeted_ind] -> cur_coro_num_);
    assert(pthread_ptr_array_[targeted_ind] -> coro_ptr_array_[cid] != NULL);
    if(pthread_ptr_array_[targeted_ind] -> coro_ptr_array_[cid] -> coro_status_ != FINISHED){
        co_wait(cid);
    }
    return pthread_ptr_array_[targeted_ind] -> coro_ptr_array_[cid] -> return_val_;
}

int co_yield(){
    //pthread_mutex_lock(&mutex_);
    int targeted_ind_in_yield;
    targeted_ind_in_yield = get_index(pthread_self());
    //pthread_mutex_unlock(&mutex_);
    assert(targeted_ind_in_yield != -1);
    assert(pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ != NULL);
    // setjmp
    int t = setjmp(pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> context_);
    // differ by return value of setjmp
    if(t == 0){
        // find the coro to yield to 
        if(pthread_ptr_array_[targeted_ind_in_yield] -> jmp_judger_ == -1){
            // choose another running coro to yield
            coroutine* targeted_coro_ptr = NULL;
            int i;
            for(i = 0; i <= pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_num_; ++i){
                if((pthread_ptr_array_[targeted_ind_in_yield] -> coro_ptr_array_[i] -> coro_id_ != pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> coro_id_) 
                && (pthread_ptr_array_[targeted_ind_in_yield] -> coro_ptr_array_[i] -> coro_status_ == RUNNING)){
                    targeted_coro_ptr = pthread_ptr_array_[targeted_ind_in_yield] -> coro_ptr_array_[i];
                    break;
                }
            }
            if(targeted_coro_ptr == NULL){ // except main coro is running
                coroutine* tmp_ptr = NULL;
                for(tmp_ptr = pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> far_coro_ptr_; tmp_ptr != NULL; tmp_ptr = tmp_ptr -> far_coro_ptr_){
                    if(tmp_ptr -> coro_status_ == RUNNING){ // must happen if cur coro is not main coro
                        targeted_coro_ptr = tmp_ptr;
                        break;
                    }
                }
            }
            if(targeted_coro_ptr == NULL){ // cur coro is main coro
                return 0;
            }
            //pthread_mutex_lock(&mutex_);
            pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ = targeted_coro_ptr;
            //pthread_mutex_unlock(&mutex_);
        }
        else{
            // yield to the targeted coro
            //pthread_mutex_lock(&mutex_);
            pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ = pthread_ptr_array_[targeted_ind_in_yield] -> coro_ptr_array_[pthread_ptr_array_[targeted_ind_in_yield] -> jmp_judger_];
            pthread_ptr_array_[targeted_ind_in_yield] -> jmp_judger_ = -1;
            //pthread_mutex_unlock(&mutex_);
        }
        // differ by new_coro_judger_
        if(pthread_ptr_array_[targeted_ind_in_yield] -> new_coro_judger_ == True){
            //pthread_mutex_lock(&mutex_);
            pthread_ptr_array_[targeted_ind_in_yield] -> new_coro_judger_ = False;
            // inline asm
            __asm__ __volatile__(
                "mov sp,%0\n\t"
                :
                :"r"(pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> coro_stack_ + STACK_SIZE-64)
                :
            );
            targeted_ind_in_yield = get_index(pthread_self());
            assert(targeted_ind_in_yield != -1);
            assert(pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ != NULL);
            //pthread_mutex_unlock(&mutex_);
            // execute function
            pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> return_val_ = (*(pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> coro_fun_ptr_))();
            //pthread_mutex_lock(&mutex_);
            pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> coro_status_ = FINISHED;
            //pthread_mutex_unlock(&mutex_);
            // roll_back cur_coro
            co_yield();
        }
        else{
            // longjmp
            longjmp(pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> context_, 2);
        }
    }
    else{
        // do nothing
    }
    return 0;
}

int co_waitall(){
    int targeted_ind = get_index(pthread_self());
    assert(targeted_ind != -1);
    int i;
    for(i = 0; i <= pthread_ptr_array_[targeted_ind] -> cur_coro_num_; ++i){
        if(pthread_ptr_array_[targeted_ind] -> coro_ptr_array_[i] -> coro_status_ != FINISHED){
            co_wait(i);
        }
    }
    return 0;
}

int co_wait(int cid){
    int targeted_ind = get_index(pthread_self());
    assert(targeted_ind != -1);
    assert(cid <= pthread_ptr_array_[targeted_ind] -> cur_coro_num_);
    assert(pthread_ptr_array_[targeted_ind] -> coro_ptr_array_[cid] != NULL);
    while(pthread_ptr_array_[targeted_ind] -> coro_ptr_array_[cid] -> coro_status_ != FINISHED){
        //pthread_mutex_lock(&mutex_);
        pthread_ptr_array_[targeted_ind] -> jmp_judger_ = cid;
        //pthread_mutex_unlock(&mutex_);
        co_yield();
    }
    return 0;
}

int co_status(int cid){
    int targeted_ind = get_index(pthread_self());
    assert(targeted_ind != -1);
    assert(cid <= pthread_ptr_array_[targeted_ind] -> cur_coro_num_);
    assert(pthread_ptr_array_[targeted_ind] -> coro_ptr_array_[cid] != NULL);
    coroutine* tmp_ptr = NULL;
    if(pthread_ptr_array_[targeted_ind] -> coro_ptr_array_[cid] == pthread_ptr_array_[targeted_ind] -> cur_coro_ptr_){
        return pthread_ptr_array_[targeted_ind] -> cur_coro_ptr_ -> coro_status_;
    }
    for(tmp_ptr = pthread_ptr_array_[targeted_ind] -> coro_ptr_array_[cid]; tmp_ptr != NULL; tmp_ptr = tmp_ptr -> far_coro_ptr_){
        if(tmp_ptr -> far_coro_ptr_ == pthread_ptr_array_[targeted_ind] -> cur_coro_ptr_) return pthread_ptr_array_[targeted_ind] -> coro_ptr_array_[cid] -> coro_status_;
    }
    return UNAUTHORIZED;
}

void check_status(){
    int targeted_ind = get_index(pthread_self());
    assert(targeted_ind != -1);
    printf("\n");
    printf("  debug: check status.\n");
    printf("  debug: in thread %d\n", targeted_ind);
    int i;
    for(i = 0; i <= pthread_ptr_array_[targeted_ind] -> cur_coro_num_; ++i){
        printf("  debug: ind : %d, coro : %d, status : %d\n", targeted_ind, i, pthread_ptr_array_[targeted_ind] -> coro_ptr_array_[i] -> coro_status_);
    }
    printf("\n");
}

void check_cur_ptr(int tag){
    int targeted_ind = get_index(pthread_self());
    assert(targeted_ind != -1);
    printf("\n");
    printf("  debug: check %d cur.\n", tag);
    printf("  debug: in thread %d\n", targeted_ind);
    assert(pthread_ptr_array_[targeted_ind] -> cur_coro_ptr_ != NULL);
    if(pthread_ptr_array_[targeted_ind] -> cur_coro_ptr_ == pthread_ptr_array_[targeted_ind] -> root_coro_ptr_){
        printf("  debug: cur is root\n");
    }
    else{
        printf("  debug: cur : %lld\n", pthread_ptr_array_[targeted_ind] -> cur_coro_ptr_ -> coro_id_);
    }
    printf("\n");
}

/*
        coroutine* targeted_ptr = NULL;
            int i;
            for(i = 0; i <= pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_num_; ++i){
                if((pthread_ptr_array_[targeted_ind_in_yield] -> coro_ptr_array_[i] -> coro_id_ != pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> coro_id_) 
                && (pthread_ptr_array_[targeted_ind_in_yield] -> coro_ptr_array_[i] -> coro_status_ == RUNNING)){
                    targeted_ptr = pthread_ptr_array_[targeted_ind_in_yield] -> coro_ptr_array_[i];
                    // printf("  debug: get yield-to-ptr not root, but %d\n", i);
                    break;
                }
            }
            if(targeted_ptr == NULL){ // except main is running
                coroutine* tmp_ptr = NULL;
                for(tmp_ptr = pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> far_coro_ptr_; tmp_ptr != NULL; tmp_ptr = tmp_ptr -> far_coro_ptr_){
                    if(tmp_ptr -> coro_status_ == RUNNING){
                        // printf("  debug: get yield-to-ptr exactly root\n");
                        targeted_ptr = tmp_ptr;
                        break;
                    }
                }
            }
            // get mutex
            pthread_mutex_lock(&mutex_);
            pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ = targeted_ptr;
            // give mutex
            pthread_mutex_unlock(&mutex_);
        */
/*
// differ by judger
    if(pthread_ptr_array_[targeted_ind_in_yield] -> jmp_judger_ == -1){ // active use co_yield, choose another running coro to yield
        if(t == 0){
            // select one coro to yield to
            coroutine* targeted_ptr = NULL;
            int i;
            for(i = 0; i <= pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_num_; ++i){
                if((pthread_ptr_array_[targeted_ind_in_yield] -> coro_ptr_array_[i] -> coro_id_ != pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> coro_id_) 
                && (pthread_ptr_array_[targeted_ind_in_yield] -> coro_ptr_array_[i] -> coro_status_ == RUNNING)){
                    targeted_ptr = pthread_ptr_array_[targeted_ind_in_yield] -> coro_ptr_array_[i];
                    // printf("  debug: get yield-to-ptr not root, but %d\n", i);
                    break;
                }
            }
            if(targeted_ptr == NULL){ // except main is running
                coroutine* tmp_ptr = NULL;
                for(tmp_ptr = pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> far_coro_ptr_; tmp_ptr != NULL; tmp_ptr = tmp_ptr -> far_coro_ptr_){
                    if(tmp_ptr -> coro_status_ == RUNNING){
                        // printf("  debug: get yield-to-ptr exactly root\n");
                        targeted_ptr = tmp_ptr;
                        break;
                    }
                }
            }
            // get mutex
            pthread_mutex_lock(&mutex_);
            pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ = targeted_ptr;
            // give mutex
            pthread_mutex_unlock(&mutex_);
            longjmp(pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> context_, 2);
        }
        else{
            // printf("  debug: jmp_judger_ == -1\n");
            return 0;
        }
    }
    else{ 
        if(t == 0){ // yield by co_start
            // get mutex
            pthread_mutex_lock(&mutex_);
            // switch cur_coro
            pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ = pthread_ptr_array_[targeted_ind_in_yield] -> coro_ptr_array_[pthread_ptr_array_[targeted_ind_in_yield] -> jmp_judger_];
            pthread_ptr_array_[targeted_ind_in_yield] -> jmp_judger_ = -1;
            // inline asm 
            __asm__ __volatile__(
                "mov sp,%0\n\t"
                :
                :"r"(pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> coro_stack_ + STACK_SIZE * 1)
                :
            );
            // give mutex
            pthread_mutex_unlock(&mutex_);
            // execute function
            // printf("  debug: at %p, ret1 : %d\n", pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_, pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> return_val_);
            pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> return_val_ = (*(pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> coro_fun_ptr_))();
            // printf("  debug: at %p, ret2 : %d\n", pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_, pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> return_val_);
            // get mutex
            pthread_mutex_lock(&mutex_);
            pthread_ptr_array_[targeted_ind_in_yield] -> cur_coro_ptr_ -> coro_status_ = FINISHED;
            // give mutex
            pthread_mutex_unlock(&mutex_);
            // roll_back cur_coro
            co_yield();
        }
        else{ // example : co_yield coro
            // printf("  debug: jmp_judger_ != -1\n");
            return 0;
        }
    }
*/

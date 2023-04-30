// ? Loc here: header modification to adapt pthread_setaffinity_np
#include "mypthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <utmpx.h>

void *thread1(void* dummy){
    assert(sched_getcpu() == 0);
    return NULL;
}

void *thread2(void* dummy){
    assert(sched_getcpu() == 1);
    return NULL;
}
int main(){
    pthread_t pid[2];
    int i;
    // ? LoC: Bind core here
    for(i = 0; i < 2; ++i){
        // 1 Loc code here: create thread and save in pid[2]
        pthread_create(&pid[i], NULL, thread1, NULL);
    }
    for(i = 0; i < 2; ++i){
        // 1 Loc code here: join thread
        pthread_join(pid[i], NULL);
    }
    return 0;
}

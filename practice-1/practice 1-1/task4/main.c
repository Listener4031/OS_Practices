// ? Loc here: header modification to adapt pthread_cond_t
#include "mypthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#define MAXTHREAD 10
// declare cond_variable: you may define MAXTHREAD variables
pthread_cond_t cond;
pthread_mutex_t mutex;

int cnt = -1;

// ? Loc in thread1: you can do any modification here, but it should be less than 20 Locs

void *thread1(void* dummy){
    pthread_mutex_lock(&mutex);
    ++cnt;
    int i;
    //printf("debug: thread %d!\n", *((int*) dummy));
    if(*((int*) dummy) == MAXTHREAD - 1){
        //printf("debug: only lock.\n");
    }
    else{
        //printf("debug: lock then wait.\n");
        pthread_cond_wait(&cond, &mutex);
    }
    printf("This is thread %d!\n", *((int*) dummy));
    for(i = 0; i < 20; ++i){
        printf("H");
        printf("e");
        printf("l");
        printf("l");
        printf("o");
        printf("W");
        printf("o");
        printf("r");
        printf("l");
        printf("d");
        printf("!");
    }
    if(*((int*) dummy) == MAXTHREAD - 1){
        //pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
        pthread_cond_broadcast(&cond);
    }
    else{
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(){
    pthread_t pid[MAXTHREAD];
    int i;
    // ? Locs: initialize the cond_variables
    pthread_cond_init(&cond, NULL);
    pthread_mutex_init(&mutex, NULL);
    for(i = 0; i < MAXTHREAD; ++i){
        int* thr = (int*) malloc(sizeof(int)); 
        *thr = i;
        // 1 Loc here: create thread and pass thr as parameter
        pthread_create(&pid[i], NULL, thread1, thr);
    }
    for(i = 0; i < MAXTHREAD; ++i)
        // 1 Loc here: join thread
        pthread_join(pid[i], NULL);
    return 0;
}

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <math.h>
#include <time.h>
#include "../include/myThread.h"

myConditionVariable cond1;
myMutexLock lock1;

int done = 1;


void *syncCheck4(void* g){

    acquire(&lock1);
    if(done == 1){
        done = 2;
        printf("Waiting on Condition Variable\n");
        myThread_cond_wait(&cond1, &lock1);
    }
    else{
        // printf("Signalling CV: %d\n", cond1.waiting->size);
        myThread_cond_signal(&cond1);
    }
    release(&lock1);
    printf("Returning thread\n");
    return NULL;
}

int main(int argc, char* argv[]){

    int a = 10;
    int* b = &a;
    myThread_cond_init(&cond1, 4);
    myThread* thread_one = malloc(sizeof(myThread));
    myThread* thread_two = malloc(sizeof(myThread));

    myThread_create(thread_one, NULL, syncCheck4, (void*) b);
    sleep(1);
    myThread_create(thread_two, NULL, syncCheck4, (void*) b);
    
    myThread_join(thread_one, NULL);
    myThread_join(thread_two, NULL);

    return 0;
}
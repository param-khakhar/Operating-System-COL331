#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <math.h>
#include <time.h>
#include "../include/myThread.h"

myMutexLock lock1;
myConditionVariable cond1;

int done = 0;

void *syncCheck5(void* g){

    for(int i=0;i<3;i++){
        printf("Printing: %d from TCB: %d\n",i,running_tcb->id);
    }
    acquire(&lock1);
    done += 1;
    while(done < 3){
        myThread_cond_wait(&cond1, &lock1);
    }
    for(int i=3;i<6;i++){
        printf("Printing: %d from TCB: %d\n",i,running_tcb->id);
    }
    myThread_cond_signal(&cond1);
    release(&lock1);
    printf("Returning thread\n");
    return NULL;
}

int main(int c, char* argv[]){

    int a = 10;
    int* b = &a;
    myThread_cond_init(&cond1, 10);
    myThread* thread_one = malloc(sizeof(myThread));
    myThread* thread_two = malloc(sizeof(myThread));
    myThread* thread_three = malloc(sizeof(myThread));

    myThread_create(thread_one, NULL, syncCheck5, (void*) b);
    myThread_create(thread_two, NULL, syncCheck5, (void*) b);
    myThread_create(thread_three, NULL, syncCheck5, (void*) b);
    
    myThread_join(thread_one, NULL);
    myThread_join(thread_two, NULL);
    myThread_join(thread_three, NULL);
    return 0;
}
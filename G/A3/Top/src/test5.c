#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <math.h>
#include <time.h>
#include "../include/myThread.h"

myMutexLock lock1;

void *syncCheck3(void* g){

    for(int i=1;i<*(int*)g;i++){
        printf("Printing: %d, From Thread: %d\n",i,running_tcb->id);
        if(i%5 == 0){
            acquire(&lock1);
            printf("Acquired Lock by: %d\n",running_tcb->id);
            printf("\nYield by: %d\n",running_tcb->id);
            myThread_yield();
            printf("Resuming: %d\n", running_tcb->id);
            printf("Releasing Lock: %d\n", running_tcb->id);
            release(&lock1);
        }
    }
    return NULL;
}

int main(int argc, char* argv[]){

    int a = 20;
    int* b = &a;

    myThread* thread_one = malloc(sizeof(myThread));
    myThread* thread_two = malloc(sizeof(myThread));

    myThread_create(thread_one, NULL, syncCheck3, (void*) b);
    myThread_create(thread_two, NULL, syncCheck3, (void*) b);
    
    myThread_join(thread_one, NULL);
    myThread_join(thread_two, NULL);

    return 0;
}
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <math.h>
#include <time.h>
#include "../include/myThread.h"

myMutexLock lock1;

void *syncCheck(void* a){

    acquire(&lock1);
    printf("Acquired Lock TCB: %d\n",running_tcb->id);
    msleep(60);
    *(int*)a += 1;
    msleep(60);
    printf("Releasing Lock TCB: %d\n",running_tcb->id);
    release(&lock1);
    return NULL;
}

int main(int c, char* argv[]){

    int* a = malloc(sizeof(int));
    *a = 1;
    myThread* thread_one = malloc(sizeof(myThread));
    myThread* thread_two = malloc(sizeof(myThread));
    myThread_create(thread_one, NULL, syncCheck, (void*) a);
    myThread_create(thread_two, NULL, syncCheck, (void*) a);

    myThread_join(thread_one, NULL);
    myThread_join(thread_two, NULL);

    printf("Updated Value: %d\n",*a);

    return 0;
}
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <math.h>
#include <time.h>
#include "../include/myThread.h"

void *syncCheck2(void* g){

    for(int i=1;i<*(int*)g;i++){
        printf("Printing: %d, From Thread: %d\n",i,running_tcb->id);
        if(i%5 == 0){
            printf("\nYield\n");
            myThread_yield();
        }
    }
    return NULL;
}

int main(int argc, char* argv[]){

    int a = 20;
    int* b = &a;
    int c = 20;
    int* d = &c;
    int e = 20;
    int* f = &e;

    myThread* thread_one = malloc(sizeof(myThread));
    myThread* thread_two = malloc(sizeof(myThread));
    myThread* thread_three = malloc(sizeof(myThread));

    myThread_create(thread_one, NULL, syncCheck2, (void*) b);
    myThread_create(thread_two, NULL, syncCheck2, (void*) d);
    myThread_create(thread_three, NULL, syncCheck2, (void*) f);
    
    myThread_join(thread_one, NULL);
    myThread_join(thread_two, NULL);
    myThread_join(thread_three, NULL);

    return 0;
}
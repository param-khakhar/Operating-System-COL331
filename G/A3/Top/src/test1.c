#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <math.h>
#include <time.h>
#include "../include/myThread.h"

void *print_message_function( void *ptr )
{
    //printf("Inside the function\n");
    char *message;
    message = (char *) ptr;
    printf("%s \n", message);
    msleep(60);
    printf("After Context Switch: TCB: %d\n", running_tcb->id);
    //printf("%d\n", running_tcb->id);
    return NULL;
}

int main(int c, char* argv[]){

    char* message1 = "Param Khakhar";
    myThread* thread_one = malloc(sizeof(myThread));
    myThread_create(thread_one, NULL, print_message_function, (void*) message1);
    myThread_join(thread_one, NULL);
    return 0;
    
}
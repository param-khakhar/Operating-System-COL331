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

int main(int argc, char* argv[]){

    char* message1 = "Param Khakhar";
    char* message2 = "Param Khakhar2";
    char* message3 = "Param Khakhar3";
    char* message4 = "Param Khakhar4";
    char* message5 = "Param Khakhar5";
    char* message6 = "Param Khakhar6";

    myThread* thread_one = malloc(sizeof(myThread));
    myThread* thread_two = malloc(sizeof(myThread));
    myThread* thread_three = malloc(sizeof(myThread));
    myThread* thread_four = malloc(sizeof(myThread));
    myThread* thread_five = malloc(sizeof(myThread));
    myThread* thread_six = malloc(sizeof(myThread));

    myThread_create(thread_one, NULL, print_message_function, (void*) message1);
    myThread_create(thread_two, NULL, print_message_function, (void*) message2);
    myThread_create(thread_three, NULL, print_message_function, (void*) message3);
    myThread_create(thread_four, NULL, print_message_function, (void*) message4);
    myThread_create(thread_five, NULL, print_message_function, (void*) message5);
    myThread_create(thread_six, NULL, print_message_function, (void*) message6);
    
    myThread_join(thread_one, NULL);
    myThread_join(thread_two, NULL);
    myThread_join(thread_three, NULL);
    myThread_join(thread_four, NULL);
    myThread_join(thread_five, NULL);
    myThread_join(thread_six, NULL);
    return 0;
}
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <math.h>
#include <time.h>
#include "../include/myThread.h"

typedef struct Item{
    int id;
}Item;

typedef struct myQueue{

    Item** q;
    int bufferSize;
    int size;
    int head;
    int tail;

}myQueue;

int push(struct myQueue* queue, Item* t){

    if(queue->size < queue->bufferSize){
        queue->q[queue->tail] = t;
        queue->tail = (queue->tail+1)%queue->bufferSize;
        queue->size++;
        return 0;
    }
    return -1;
}

int pop(struct myQueue* queue){

    if(queue->size > 0){
        queue->head = (queue->head + 1) %queue->bufferSize;
        queue->size--;
        return 0;
    }
    return -1;
}

Item* front(myQueue* queue){
    return queue->q[queue->head];
}

myQueue* inventory;
myConditionVariable cond;
myMutexLock lock;
int consumers;
int producers;
int buffer;
int total;
int itemID;

void* producer(void* a){

    while(total < 10 * buffer){
        acquire(&lock);
        Item* curr;
        curr = (Item*)malloc(sizeof(Item));
        curr->id = itemID++;
        while(inventory->size == inventory->bufferSize && total < 10*buffer){
            myThread_cond_wait(&cond, &lock);
        }
        if(total >= 10*buffer){
            release(&lock);
            char* msg = "Exit";
            myThread_cond_signal(&cond);
            myThread_exit((void*)msg);
        }
        push(inventory, curr);
        printf("Item produced by Producer: %d\n",running_tcb->id);
        total += 1;
        printf("Total Transactions: %d, Current size of inventory: %d\n", total, inventory->size);
        myThread_cond_signal(&cond);
        release(&lock);
    }
}

void* consumer(void* a){
    while(total < 10 * buffer){
        acquire(&lock);
        while(inventory->size == 0 && total < 10*buffer){
            myThread_cond_wait(&cond, &lock);
        }
        if(total >= 10*buffer){
            release(&lock);
            char* msg = "Exit";
            myThread_cond_signal(&cond);
            myThread_exit((void*)msg);
        }

        Item* curr;
        curr = front(inventory);
        printf("Item acquired by Consumer: %d\n",running_tcb->id);
        pop(inventory);
        total += 1;
        printf("Total Transactions: %d, Current size of inventory: %d\n", total, inventory->size);
        myThread_cond_signal(&cond);
        release(&lock);
    }
}

int main(int c, char* argv[]){

    total = 0;
    itemID = 0;
    consumers = producers = atoi(argv[1]);
    buffer = atoi(argv[2]);
    printf("Consumers: %d, Producers: %d, Buffer: %d\n", consumers, producers, buffer);

    myThread** prod = (myThread**)malloc(sizeof(myThread*)*producers);
    myThread** cons = (myThread**)malloc(sizeof(myThread*)*consumers);
    int* a;

    myThread_cond_init(&cond, 2*producers);

    inventory = malloc(sizeof(myQueue));
    inventory->bufferSize = buffer;
    inventory->q = malloc(sizeof(Item*)*inventory->bufferSize);

    for(int i=0;i<consumers;i++){

        prod[i] = (myThread*)malloc(sizeof(myThread));
        cons[i] = (myThread*)malloc(sizeof(myThread));

        myThread_create(prod[i], NULL, producer, (void*)a);
        myThread_create(cons[i], NULL, consumer, (void*)a);
    }

    for(int i=0;i<consumers;i++){
        myThread_join(prod[i], NULL);
        myThread_join(cons[i], NULL);
    }
	return 0; 
} 
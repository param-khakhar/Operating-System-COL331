#include "../include/queue.h"

extern int qpush(struct Queue* queue, struct myThread* t){

    if(queue->size < queue->maxThreads){
        queue->q[queue->tail] = t;
        queue->tail = (queue->tail+1)%queue->maxThreads;
        queue->size++;
        return 0;
    }
    return -1;
}

extern int qpop(struct Queue* queue){

    if(queue->size > 0){
        queue->head = (queue->head + 1) %queue->maxThreads;
        queue->size--;
        return 0;
    }
    return -1;
}

extern struct myThread* getHead(struct Queue* queue){
    return queue->q[queue->head];
}
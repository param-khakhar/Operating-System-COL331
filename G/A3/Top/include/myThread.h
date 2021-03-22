#include<setjmp.h>

#ifndef myThread_H
#define myThread_H

typedef struct myThread_attr{
    // Information about the threads to be put in this region.
    size_t stack_size;                 // Integer representing stack size;
    int* sp;                        // Address to the top of the stack;
    int* sb;                        // Address to the bottom of the stack;
    void* stack;

}myThread_attr;

typedef struct myThread{
    // Information about the threads to be put in this region.

    size_t stack_size;                 // Integer representing stack size;
    int status;                     // 0 implies function incomplete, 1 implies function complete, 2 implies wait in a queue of condition variable.
    int join;                       // 0 implies not joined, 1 implies delayed.
    int id;
    //int* sp;
    void* stackaddr;
    myThread_attr* attributes;
    void* (*function)(void*);
    void* arg;
    jmp_buf context;
    int first;                     // first = 0, implies function to be called, whereas, first = 1, implies restore context.
    struct myThread* joining_thread;
    int cancel;                     // cancel = 1, implies cancellation orders.

}myThread;

typedef struct myMutexLock{
    int guard;
    int value;
}myMutexLock;

typedef struct Queue{

    struct myThread** q;
    int size;
    int head;
    int tail;
    int maxThreads;
}Queue;

typedef struct myConditionVariable{
    Queue* waiting;
    myMutexLock lock;
    int id; 
}myConditionVariable;

extern int global_id;
extern int flagScheduler;                  
extern int flagInit;                       // Need to call Init only once.
extern int setitimer();

extern int sigaction();
extern struct sigaction sa;                    // Struct for the signal handler
extern struct itimerval timer;                 // Struct for using setitimer

extern sigset_t blocksigs;                     // Set of Signals to be blocked.

extern jmp_buf schedulerContext;               // Context of the Scheduler, to be used when switching threads.
extern jmp_buf mainContext;                    // Context of the function main().

extern struct Queue* ready;
extern int readysize;
extern myThread* running_tcb;
extern myThread* main_tcb;

extern int myThread_cond_init(myConditionVariable* cond, int size);

extern int myThread_cond_wait(myConditionVariable* cond, myMutexLock* lock);

extern int myThread_cond_signal(myConditionVariable* cond);

extern int myThread_cond_brodcast(myConditionVariable* cond);

extern int msleep(long msec);

extern void acquire(myMutexLock* lock);

extern void release(myMutexLock* lock);

extern char cas(int* ptr, int old, int newV);

extern int myThread_attr_destroy(myThread_attr* attr);

extern int myThread_attr_init(myThread_attr* attr);

extern int myThread_attr_setstack(myThread_attr* thread_attr, void* stackaddr, size_t stacksize);

extern int myThread_attr_getstack(myThread_attr*, void** stackaddr, size_t* stacksize);

extern int myThread_attr_setstackaddr(myThread_attr* thread_attr, void* addr);

extern int myThread_attr_getstackaddr(myThread_attr* thread_attr, void** addr);

extern int myThread_attr_setstacksize(myThread_attr* thread_attr, size_t stacksize);

extern int myThread_attr_getstacksize(myThread_attr* thread_attr, size_t* stacksize);

extern int myThread_create(myThread* thread, struct myThread_attr* thread_attr, void*(*start_routine)(void*), void* args);

extern void myThread_exit(void* status);

extern int myThread_cancel(myThread* thread);

extern int myThread_attr_init(myThread_attr* attr);

extern int myThread_attr_destroy(myThread_attr* attr);

extern void myThread_yield(void);

extern int myThread_join(myThread* thread, void** value_ptr);

extern void myThreadswitch();

extern myThread myThread_self(void);

extern void scheduler();

extern void init(myThread* firstTCB);

extern void enableInterrupts();

extern void disableInterrupts();

extern int qpush(struct Queue* queue, struct myThread* t);

extern int qpop(struct Queue* queue);

extern struct myThread* getHead(struct Queue* queue);

#endif
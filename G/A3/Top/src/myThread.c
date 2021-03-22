#include<stdio.h>
#include<setjmp.h>
#include<signal.h>
#include<stdlib.h>
#include<sys/time.h>
#include<string.h>
#include<unistd.h>
#include<time.h>
#include <errno.h>
#include "../include/myThread.h"    

#define MAX_THREADS 100
#define INITIAL_STACK_SIZE 16 * 1024
#define TIMER_INTERVAL 50000

#define NEW_TASK 1
#define JOIN_THREAD 1
#define HANDLE_INTERRUPT 1

void scheduler();

int global_id = 0;
int flagScheduler = 0;                  
int flagInit = 0;                       // Need to call Init only once.

int setitimer();

int sigaction();
struct sigaction sa;                    // Struct for the signal handler
struct itimerval timer;                 // Struct for using setitimer

sigset_t blocksigs;                     // Set of Signals to be blocked.

jmp_buf schedulerContext;               // Context of the Scheduler, to be used when switching threads.
jmp_buf mainContext;                    // Context of the function main().

struct Queue* ready;
int readysize;
struct Queue* wait;
int waitsize;
myThread* running_tcb;
myThread* main_tcb;

myMutexLock lock1;
myConditionVariable cond1;

int qpush(struct Queue* queue, struct myThread* t){

    if(queue->size < queue->maxThreads){
        queue->q[queue->tail] = t;
        queue->tail = (queue->tail+1)%queue->maxThreads;
        queue->size++;
        return 0;
    }
    return -1;
}

int qpop(struct Queue* queue){

    if(queue->size > 0){
        queue->head = (queue->head + 1) %queue->maxThreads;
        queue->size--;
        return 0;
    }
    return -1;
}

struct myThread* getHead(struct Queue* queue){
    return queue->q[queue->head];
}

/* msleep(): Sleep for the requested number of milliseconds.*/
int msleep(long msec)
{
    struct timespec ts;
    int res;

    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    res = nanosleep(&ts, &ts);

    return res;
}

char cas(int *ptr, int old, int newV) {
    unsigned char ret;
    // Note that sete sets a ’byte’ not the word
    asm volatile(
	" lock\n"
	" cmpxchgl %2,%1\n"
	" sete %0\n"
	: "=q" (ret), "=m" (*ptr)
	: "r" (newV), "m" (*ptr), "a" (old)
	: "memory");
    return ret;
}

// void acquire (myMutexLock* lock){
//     while(!cas(&(lock->guard),0,1));
//     if(lock->value == 1){
//         qpush(wait, running_tcb);
//         guard = 0;
//     }
//     else{
//         value = 1;
//         guard = 0;
//     }
// }

// void release(myMutexLock* lock){

//     while(!cas(&(lock->guard),0,1));
//     if(wait->size>0){
//         qpush(wait, running_tcb);
//         guard = 0;
//     }
//     else{
//         value = 1;
//         guard = 0;
//     }

// }

void acquire(myMutexLock* lock){
    while(lock->value);
    while(!cas(&(lock->value),0,1));
}

void release(myMutexLock* lock){
    lock->value = 0;
}

int myThread_cond_wait(myConditionVariable* cond, myMutexLock* lock){
    if(cond){
        acquire(&(cond->lock));
        //printf("Pushed Inside!\n");
        qpush(cond->waiting, running_tcb);
        //printf("Condition Waiting Size: %d\n", cond->waiting->size);
        release(&(cond->lock));
        // Disabling Interrupts to ensure atomicity
        disableInterrupts();
        release(lock);

        if(setjmp(running_tcb->context)==0){
            running_tcb->status = 2;
            longjmp(schedulerContext, 1);
        }
        else{
            acquire(lock);
        }
    }
    return -1;
}

int myThread_cond_signal(myConditionVariable* cond){

    if(cond){
        acquire(&cond->lock);
        if(cond->waiting->size > 0){
            myThread* temp = getHead(cond->waiting);
            qpop(cond->waiting);
            qpush(ready, temp);
        }
        release(&cond->lock);
        return 0;
    }
    return -1;
}

int myThread_cond_brodcast(myConditionVariable* cond){
    
    if(cond){
        acquire(&cond->lock);
        while(cond->waiting->size > 0){
            myThread* temp = getHead(cond->waiting);
            qpop(cond->waiting);
            qpush(ready, temp);
        }
        release(&cond->lock);
        return 0;
    }
    return -1;
}

int myThread_cond_init(myConditionVariable* cond, int size){
    if(cond){
        cond->waiting = (Queue*)malloc(sizeof(Queue));
        cond->waiting->q = malloc(sizeof(myThread*)*size);
        cond->waiting->maxThreads = size;
        return 0;
    }
    return -1;
}

void enableInterrupts(){
    //Start the timer, bu unblocking the SIGALRM from now onwards.
    if(sigprocmask(SIG_UNBLOCK, &blocksigs, NULL) == -1){
        printf("Error in Start\n");
    }
}

void disableInterrupts(){
    //Stop the timer, by blocking the SIGALRM from now onwards.
    if(sigprocmask(SIG_BLOCK, &blocksigs, NULL) == -1){
        printf("Error in Start\n");
    }
}

void myThreadSwitch(){

    //printf("Interrupt Handler: %d\n",ready->size);
    disableInterrupts();                 // We don't want a context switch here.

    //Saving the context of the current thread in execution. Need to go to the scheduler as it will schedule a new thread.
    if(setjmp(running_tcb->context) == 0){
        // printf("Switching from: %d %d\n",running_tcb->id, ready->size);
        longjmp(schedulerContext, 1);
    }
    else{
        disableInterrupts();
        // printf("Here7, size of queue: %d %d\n", ready->size,running_tcb->id);
        enableInterrupts();
    }
}


void init(myThread* firstTCB){

    ready = (struct Queue*)malloc(sizeof(struct Queue));
    ready->q = malloc(sizeof(myThread*)*100);
    ready->maxThreads = 100;
    wait = (struct Queue*)malloc(sizeof(struct Queue));
    wait->maxThreads = 100;
    main_tcb = (myThread*)malloc(sizeof(myThread));

    main_tcb->id = -1;
    main_tcb->join = 0;
    qpush(ready, main_tcb);
    qpush(ready, firstTCB);
    running_tcb = main_tcb;
    //printf("Pushed Main\n");

    memset(&sa,0,sizeof(sa));                       //Initializing the signal handler.
    sa.sa_handler = &myThreadSwitch;                //Function for handling interrupts.
    sa.sa_flags = SA_NODEFER;                       //Do not prevent the signal to be received within it's own handler?
    sigaction(SIGALRM, &sa, NULL);                  //Initializing the sigaction struct responsible for signalling the scheduler.

    timer.it_value.tv_sec = 0;                      //Parameters for setitimer and itimerval class
    timer.it_value.tv_usec = TIMER_INTERVAL;        
    timer.it_interval.tv_sec = 0;                   
    timer.it_interval.tv_usec = TIMER_INTERVAL;
    setitimer (ITIMER_REAL, &timer, NULL);          //Calling the setitimer function.

    if (sigaddset(&blocksigs, SIGALRM) == -1){
		perror("sigaddset error");
		return ;
	}
    scheduler();
}

int myThread_create(struct myThread* thread, struct myThread_attr* thread_attr, void*(*start_routine)(void*), void* args){
    
    disableInterrupts();

    thread->function = start_routine;
    thread->arg = args;
    thread->first = 0;
    thread->cancel = 0;

    if(thread_attr == NULL){
        myThread_attr* temp = (myThread_attr*)malloc(sizeof(myThread_attr));
        myThread_attr_init(temp);
        thread->attributes = temp;
        thread->stackaddr = (void*)malloc(INITIAL_STACK_SIZE);
        //thread->stack_size = INITIAL_STACK_SIZE;
        //thread->sp = thread->stack + INITIAL_STACK_SIZE;
        // printf("%d\n",sizeof(thread->attributes->stack));
    }
    else
        thread->attributes = thread_attr;

    thread->id = global_id;
    thread->status = 0;
    global_id++;

    if(setjmp(mainContext) == 0){
        if(flagInit == 0){
            flagInit = 1;
            init(thread);
        }
        else
            qpush(ready, thread);
    }
    enableInterrupts();
    // printf("Creation Complete %d\n",global_id-1);   
    return global_id-1;
}

void *print_message_function( void *ptr )
{
    //printf("Inside the function\n");
    char *message;
    message = (char *) ptr;
    printf("%s \n", message);
    //msleep(60);
    printf("After Context Switch: TCB: %d\n", running_tcb->id);
    //printf("%d\n", running_tcb->id);
    return NULL;
}

void schedule(){
    if(running_tcb->first == 0){
        (*running_tcb).first = 1;

        enableInterrupts();
        register void *top = running_tcb->attributes->sp;
        asm volatile(
            "mov %[rs], %%rsp \n"
            : [ rs ] "+r" (top) ::
        );
        (running_tcb->function)(running_tcb->arg);

        disableInterrupts();
        (*running_tcb).status = 1;
        if(running_tcb->joining_thread != NULL)
            (*(*running_tcb).joining_thread).join -= 1;

        // free(running_tcb->stackaddr);
        // free(running_tcb->attributes);
        longjmp(schedulerContext,NEW_TASK);
    }
    else{
        enableInterrupts();
        longjmp(running_tcb->context,1);
    }
}

void scheduler(){

    //printf("Inside Schedular\n");
    disableInterrupts();
    if(setjmp(schedulerContext) == 0){
        if(ready->size > 0){
            qpop(ready);
            qpush(ready, running_tcb);
            running_tcb = getHead(ready);

            if(running_tcb->id != -1){
                schedule(running_tcb);
            }
        }
    }
    else{
        //Schedule a new Task, Handle an interrupt, 
        // printf("Old: %d %d\n",running_tcb->id, ready->size);
        if((running_tcb->status == 1) || (running_tcb->status == 2)){
            qpop(ready);
            running_tcb = getHead(ready);
        }
        else{
            qpop(ready);
            qpush(ready, running_tcb);
            running_tcb = getHead(ready);
        }
        // printf("New: %d %d\n",running_tcb->id, ready->size);
        if(running_tcb->id == -1){
            //Main Thread
            if(running_tcb->join >= 1){
                longjmp(schedulerContext, 1);
            }
            else{
                //enableInterrupts();
                longjmp(mainContext,1);
            }
        }
        else{
            schedule();
        }
    }
}

void myThread_exit(void* status){
    running_tcb->status = 1;
    if(running_tcb->joining_thread){
        running_tcb->joining_thread->join -= 1;
    }
    longjmp(schedulerContext, NEW_TASK);
}

int myThread_attr_setstack(myThread_attr* thread_attr, void* stackaddr, size_t stacksize){
    if(thread_attr){
        thread_attr->stack = stackaddr;
        thread_attr->stack_size = stacksize;
        return 0;
    }
    return -1;
}

int myThread_attr_getstack(myThread_attr* thread_attr, void** stackaddr, size_t* stacksize){
    if(thread_attr){
        *stackaddr = thread_attr->stack;
        *stacksize = thread_attr->stack_size;
        return 0;
    }
    return -1;
}

int myThread_attr_setstackaddr(myThread_attr* thread_attr, void* addr){
    if(thread_attr){
        thread_attr->stack = addr;
        return 0;
    }
    return -1;
}

int myThread_attr_getstackaddr(myThread_attr* thread_attr, void** addr){
    if(thread_attr){
        *addr = thread_attr->stack;
        return 0;
    }
    return -1;
}

int myThread_attr_setstacksize(myThread_attr* thread_attr, size_t stacksize){
    if(thread_attr){
        thread_attr->stack_size = stacksize;
        return 0;
    }
    return -1;
}

int myThread_attr_getstacksize(myThread_attr* thread_attr, size_t* stacksize){
    if(thread_attr){
        *stacksize = thread_attr->stack_size;
    }
    return -1;
}

int myThread_cancel(myThread* thread){
    if(thread){
        thread->cancel = 1;
        return 0;
    }
    return -1;
}

int myThread_attr_init(myThread_attr* attr){
    if(attr){
        attr->stack_size = INITIAL_STACK_SIZE;
        attr->stack = (void*)malloc(sizeof(INITIAL_STACK_SIZE));
        attr->sp = attr->stack + attr->stack_size;
        return 0;
    }
    return -1;
}

int myThread_attr_destroy(myThread_attr* attr){
    if(attr){
        attr->stack_size = 0;
        attr->stack = NULL;
        attr->sp = NULL;
        return 0;
    }
    return -1;
}

void myThread_yield(){

    disableInterrupts();
    if(setjmp(running_tcb->context) == 0)
        longjmp(schedulerContext, NEW_TASK);
    else
        enableInterrupts();
}

int myThread_join(myThread* thread, void** value_ptr){
    
    disableInterrupts();
    if(thread->status != 1){
        thread->joining_thread = running_tcb;
        running_tcb->join = running_tcb->join + 1;
        longjmp(schedulerContext, JOIN_THREAD);
    }
    enableInterrupts();

    return 1;
}

myThread myThread_self(void){
    return *running_tcb;
}

void singleThread(){
    char* message1 = "Param Khakhar";
    myThread* thread_one = malloc(sizeof(myThread));
    myThread_create(thread_one, NULL, print_message_function, (void*) message1);
    myThread_join(thread_one, NULL);
}

void multipleThreads(){

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
}


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


void multipleThreadsLock(){

    int* a = malloc(sizeof(int));
    *a = 1;
    myThread* thread_one = malloc(sizeof(myThread));
    myThread* thread_two = malloc(sizeof(myThread));
    myThread_create(thread_one, NULL, syncCheck, (void*) a);
    myThread_create(thread_two, NULL, syncCheck, (void*) a);

    myThread_join(thread_one, NULL);
    myThread_join(thread_two, NULL);

    printf("Updated Value: %d\n",*a);
}

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

void multipleThreadsYield(){

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
}

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

void multipleThreadsLockYield(){

    int a = 20;
    int* b = &a;

    myThread* thread_one = malloc(sizeof(myThread));
    myThread* thread_two = malloc(sizeof(myThread));

    myThread_create(thread_one, NULL, syncCheck3, (void*) b);
    myThread_create(thread_two, NULL, syncCheck3, (void*) b);
    
    myThread_join(thread_one, NULL);
    myThread_join(thread_two, NULL);

}

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

void multipleThreadsCV(){

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
}

void multipleThreadsCVyield(){
    
}

// int main(int c, char* argv[]){

//     char* message1 = "Param Khakhar";
//     char* message2 = "Param Khakhar2";
//     char* message3 = "Param Khakhar3";
//     char* message4 = "Param Khakhar4";
//     char* message5 = "Param Khakhar5";
//     char* message6 = "Param Khakhar6";

//     myThread* thread_one = malloc(sizeof(myThread));
//     myThread* thread_two = malloc(sizeof(myThread));
//     myThread* thread_three = malloc(sizeof(myThread));

//     myThread_create(thread_one, NULL, OMG, (void*) message1);
//     myThread_create(thread_two, NULL, OMG, (void*) message2);
//     myThread_create(thread_three, NULL, OMG, (void*) message3);

    
//     myThread_join(thread_one, NULL);
//     myThread_join(thread_two, NULL);
//     myThread_join(thread_three, NULL);
// 	return 0;
// }

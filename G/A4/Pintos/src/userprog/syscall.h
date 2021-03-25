#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"
#include "threads/thread.h"

#define ERROR -1
#define STDOUT 1

void syscall_init (void);

struct file_aux{
  struct file* file;
  int fd;
  struct list_elem elem;
};

struct write_args{
    int id;
    int fd;
    const void* buffer;
    unsigned size;
};

struct lock file_lock;

#endif /* userprog/syscall.h */

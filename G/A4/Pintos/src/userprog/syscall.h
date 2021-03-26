#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"
#include "threads/thread.h"
#include <user/syscall.h>

#define ERROR -1
#define STDOUT 1
#define STDIN 0

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

struct open_args{
  int id;
  const char* file;
};

struct create_args{
  int id;
  const char* file;
  unsigned initial_size;
};

struct close_args{
  int id;
  int fd;
};

struct seek_args{
  int id;
  int fd;
  unsigned position;
};

struct fileSize_args{
  int id;
  int fd;
};

struct read_args{
  int id;
  int fd;
  const void* buffer;
  unsigned size;
};

struct remove_args{
  int id;
  const char* file;
};

struct exec_args{
  int id;
  const char* cmd_line;
};

struct exit_args{
  int id;
  int status;
};

struct wait_args{
  int id;
  pid_t child;
};

struct lock file_lock;

#endif /* userprog/syscall.h */

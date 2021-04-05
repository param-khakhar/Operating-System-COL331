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

struct childProcess{
  int pid;                          //Pid of the child process
  struct list_elem elem;            //List element for insertion deletion
  int waiting;                      //Waiting = 1, indicates that wait has already been called once.
  struct semaphore waitLock;        //Inherited from the parent, used when parent waits for the child.
  int exit_status;                         //Attribute indicating whether the thread has exited or not.
  int status;                       //Status to be returned to the parent process.
  int load_status;                         //Whether the child has loaded correctly or not.
};

struct lock file_lock;
struct childProcess* searchChild(tid_t tid);
void removeChild(pid_t id, bool all);
void sys_close(int fileDes, bool close_all);
#endif /* userprog/syscall.h */

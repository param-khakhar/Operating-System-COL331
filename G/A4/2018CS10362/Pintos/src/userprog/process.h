#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"
#include "threads/synch.h"

struct sync_aux{
  const char* cmd_line;
  struct semaphore sem;
  char* fn_clone;
};

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

/* exception.c uses this */
bool install_page (void *upage, void *kpage, bool writable);
#endif /* userprog/process.h */

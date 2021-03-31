#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "userprog/exception.h"
#include "vm/sup_page.h"
#include "vm/frame.h"
#include <user/syscall.h>

static void syscall_handler (struct intr_frame *);
bool lock_flag = true;

/* Syscalls */
void sys_close(int fd, bool close_all);
bool sys_create(const char* file, unsigned initial_size);
pid_t sys_exec(const char* cmd_line);
void sys_exit(int status);
int sys_fileSize(int fd);
void sys_halt(void);
int sys_open(const char* name);
int sys_read(int fd, void* buffer, unsigned size);
bool sys_remove(const char* name);
void sys_seek(int fd, unsigned position);
int sys_wait(pid_t pid);
int sys_write(int fd, void* buffer, unsigned size);

/* Auxiliary Functions*/

struct file* getFile(int fileD);
int insert_file(struct file* f);
void check(void* sp, int desired);
void check_str(const void* str);
void check_buffer(const void* buffer, unsigned size);
void* convert(const void* loc);
void valid_address(const void* addr);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/* Returns true if the user address sequence is in valid range or false otherwise.
   If exact is true, the whole range is checked, otherwise this can be used to
   check for validity of strings - it only looks up to end of string \0.
 */
bool validate_user_addr_range(uint8_t *va, size_t bcnt, uint32_t* esp, bool exact);

/* Uses the second technique mentioned in pintos doc. 3.1.5 
   to cause page faults and check addresses (returns -1 on fault) */

/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */
static int
get_user (const uint8_t *uaddr) {
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:" : "=&a" (result) : "m" (*uaddr));
  return result;
}

/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
/*
static bool
put_user (uint8_t *udst, uint8_t byte) {
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:" : "=&a" (error_code), "=m" (*udst) : "q" (byte));
  return error_code != -1;
}
*/
/* Used to validate pointers, buffers and strings. 
   With exact false, it validates until 0 (end of string). */
bool validate_user_addr_range(uint8_t *va, size_t bcnt, uint32_t* esp, bool exact) {
  if(va == NULL)  /* NULL is not allowed */
    return false;
  for(size_t i=0; (i<bcnt) || !exact; i++) {
    if(!is_user_vaddr(va+i)) /* outside user space! wrong */
      return false;
    int w = get_user(va+i);
    if(!exact && w == 0) /* end of string */
      return true;
    if(w == -1) { /* outside mapped pages */
#ifdef VM
      uint8_t* uaddr = PFNO_TO_ADDR(ADDR_TO_PFNO(va+i));
      struct sup_page* spg = lookup_page(uaddr);
      if(spg != NULL && load_page(spg, uaddr)) /* page must be loaded */
        continue; /* check next address */
      if(va+i > (uint8_t*)esp && grow_stack(uaddr)) /* 1st stack access in syscall */
        continue; /* check next address*/
      /* none of these situations! */
#endif
      return false;
    }
  }
  return true;
}

/* File system primitive synchronization. Sequentialize file system accesses. */
#define FS_ATOMIC(code) \
  {  fs_take(); \
     {code} \
     fs_give(); }

static void
syscall_handler (struct intr_frame *f) 
{
  if(lock_flag){
    lock_init(&file_lock);
    lock_flag = false;
  }

  // printf("System calls not implemented correctly.\n");

  // void* stack_pointer;
  int syscall_number;
  void* arg;
  syscall_number = *(int *)convert(f->esp);

  switch(syscall_number){

    case SYS_CREATE:

      // printf("Create System Call\n");
      check(f->esp, 2);
      struct create_args* create = (struct create_args*)f->esp;
      check_str(create->file);
      arg = convert(create->file);
      f->eax = sys_create(arg, create->initial_size);
      break;

    case SYS_REMOVE:

      // printf("Remove System Call\n");
      check(f->esp, 1);
      struct remove_args* remove = (struct remove_args*)f->esp;
      check_str(remove->file);
      arg = convert(remove->file);
      f->eax = sys_remove(arg);
      break;

    case SYS_OPEN:

      // printf("Open System Call\n");
      check(f->esp, 1);
      struct open_args* open = (struct open_args*)f->esp;
      check_str(open->file);
      arg = convert(open->file);      
      f->eax = sys_open(arg);
      break;

    case SYS_CLOSE:

      // printf("Close System Call\n");
      check(f->esp,1);
      struct close_args* close = (struct close_args*)f->esp;
      sys_close(close->fd, false);
      break;

    case SYS_READ:

      // printf("Read System Call\n");
      check(f->esp,3);
      struct read_args* read = (struct read_args* )f->esp;
      check_buffer(read->buffer, read->size);
      arg = convert(read->buffer);
      f->eax = sys_read(read->fd, arg, read->size);
      break;

    case SYS_WRITE:

      // printf("Write System Call\n");
      check(f->esp, 3);
      struct write_args* write = (struct write_args *)f->esp;
      check_buffer(write->buffer, write->size);
      arg = convert(write->buffer);
      f->eax = sys_write(write->fd, arg, write->size);
      break;

    case SYS_SEEK:

      // printf("Seek System Call\n");
      check(f->esp, 2);
      struct seek_args* seek = (struct seek_args* )f->esp;
      sys_seek(seek->fd, seek->position);
      break;

    case SYS_FILESIZE:

      // printf("FileSize system Call\n");
      check(f->esp, 1);
      struct fileSize_args* fileSize = (struct fileSize_args*)f->esp;
      f->eax = sys_fileSize(fileSize->fd);
      break;

    case SYS_HALT:
      // printf("Halt System Call\n");
      sys_halt();
      break;
    
    case SYS_EXIT:
      // printf("Exit System Call\n");
      check(f->esp,1);
      struct exit_args* exit = (struct exit_args*)f->esp;
      printf("%s: %s\n",thread_name(), "exit(%d)",exit->status);
      sys_exit(exit->status);
      break;

    case SYS_EXEC:
      // printf("Exec System Call\n");
      check(f->esp, 1);
      struct exec_args* exec = (struct exec_args*)f->esp;
      arg = convert(exec->cmd_line);
      sys_exec(arg);
      break;

    case SYS_WAIT:
      // printf("Wait System Call\n");
      check(f->esp, 1);
      struct wait_args* wait = (struct wait_args*)f->esp;
      f->eax = sys_wait(wait->child);
      break;

    default:
      printf("Default!\n");
      break;
  }
  // thread_exit();
}

int sys_open(const char* name){
  /* Auxiliary function for opening the file */
  lock_acquire(&file_lock);
  struct file *temp = filesys_open(name);
  lock_release(&file_lock);

  if(!temp){
    return -1;
  }
  int fd = insert_file(temp);
  return fd;
}

bool sys_remove(const char* name){
  lock_acquire(&file_lock);
  bool res = filesys_remove(name);
  lock_release(&file_lock);
  return res;
}

int sys_write(int fd, void* buffer, unsigned size){

  if(size > 0){
    if(fd == STDOUT){
      putbuf(buffer, size);
      return size;
    }
    else{
      struct file* fileptr = getFile(fd);
      if(!fileptr){
        return ERROR;
      }
      lock_acquire(&file_lock);
      int written = file_write(fileptr, buffer, size);
      lock_release(&file_lock);
      return written;
    }
  }
  return size;  
}

void sys_seek(int fd, unsigned position){

  struct file* fileptr = getFile(fd);
  if(!fileptr){
    return;
  }
  lock_acquire(&file_lock);
  file_seek(fileptr, position);
  lock_release(&file_lock);
}

int sys_read(int fd, void* buffer, unsigned size){

  if(size > 0){
    if(fd == STDIN){
      uint8_t* local_buff = (uint8_t*)buffer;
      for(unsigned i=0; i<size;i++){
        local_buff[i] = input_getc();
      }  
      return size;
    }
    else{
      lock_acquire(&file_lock);
      struct file* fileptr = getFile(fd);
      if(!fileptr){
        lock_release(&file_lock);
        return ERROR;
      }
      int read = file_read(fileptr, buffer, size);
      lock_release(&file_lock);
      return read;
    }
  }
  return size;  
}


bool sys_create(const char* file, unsigned initial_size){
  lock_acquire(&file_lock);
  bool res = filesys_create(file, initial_size);
  lock_release(&file_lock);
  return res;
} 

void sys_halt(){
  shutdown_power_off();
}

void sys_close(int fileDes, bool close_all){

  struct thread* curr = thread_current();
  struct list_elem* head = list_begin(&curr->file_list);

  while(head != list_end(&curr->file_list)){
    struct file_aux* fa = list_entry(head, struct file_aux, elem);
    if(close_all){
      lock_acquire(&file_lock);
      file_close(fa->file);
      lock_release(&file_lock);
      list_remove(&fa->elem);
      free(fa);
    }
    else if(fa->fd == fileDes){
      lock_acquire(&file_lock);
      file_close(fa->file);
      lock_release(&file_lock);
      list_remove(&fa->elem);
      free(fa);
      return;
    }
    head = list_next(head);
  }
}

int sys_fileSize(int fd){
  struct file* fileptr = getFile(fd);
  if(!fileptr){
    return ERROR;
  }
  lock_acquire(&file_lock);
  int size = file_length(fileptr);
  lock_release(&file_lock);
  return size;
}

int sys_wait(pid_t pid){

  return process_wait(pid);
}

pid_t sys_exec(const char* cmd_line){

  pid_t childPid = process_execute(cmd_line);
  struct childProcess* child = searchChild(childPid);
  if(!child || !child->load){
    return ERROR;
  }
  return childPid;
}

void sys_exit(int status){
  struct thread* t = thread_current();
  if(findThread(t->parent) && t->corresp){
    if(status < 0)
      status = -1;
    t->corresp->status = status;
  }
  sema_up(&t->corresp->waitLock);
  thread_exit();
}

struct file* getFile(int fileD){

  struct thread* curr = thread_current();
  struct list_elem* head = list_begin(&curr->file_list);

  while(head != list_end(&curr->file_list)){
    struct file_aux* fa = list_entry(head, struct file_aux, elem);
    if(fa->fd == fileD){
      return fa->file;
    }
    head = list_next(head);
  }
  return NULL;
}

int insert_file(struct file* f){
  struct file_aux* temp = malloc(sizeof(struct file_aux));
  if(!temp){
    return ERROR;
  }
  struct thread* running = thread_current();

  temp->file = f;
  temp->fd = running->fd++;
  list_push_back(&running->file_list, &temp->elem);
  return temp->fd;
}

void check(void* sp, int desired){
  int *addr;
  for(int i=0;i<=desired;i++){
    addr = (int*)sp + i;
    valid_address((const void*)addr);
  }
}

void check_buffer(const void* buffer, unsigned size){
  char* curr = (char*)buffer;
  for(unsigned i=0;i<size;i++){
    valid_address((const void*)curr);
    curr++; 
  }
}

void check_str(const void* file){
  /* Checked whether the virtual user address is mapped or unmapped for the characters of the string.*/
  while(!(char*)convert(file)){
    file = (char*)file + 1;
  }
}

void* convert(const void* loc){
  /*Converts the pointer loc to an address mapped in kernel virtual memory*/
  void* kernel_addr = pagedir_get_page(thread_current()->pagedir, (loc));
  if(!kernel_addr){
    sys_exit(ERROR);
  }
  return kernel_addr;
}

void valid_address(const void* addr){
  if(!is_user_vaddr(addr) || addr >= PHYS_BASE){
    sys_exit(ERROR);
  }
}

void removeChild(pid_t id, bool all){

  struct thread* curr = thread_current();
  struct list_elem* head = list_begin(&curr->children);

  while(head != list_end(&curr->children)){
    struct childProcess* c = list_entry(head, struct childProcess, elem);
    if(all){
      list_remove(&c->elem);
      free(c);
    }
    else if(c->pid == id){
      list_remove(&c->elem);
      free(c);
      return;
    }
    head = list_next(head);
  }
}

struct childProcess* searchChild(tid_t tid){

  struct thread* curr = thread_current();
  struct list_elem* head = list_begin(&curr->children);

  while(head != list_end(&curr->children)){
    struct childProcess* c = list_entry(head, struct childProcess, elem);
    if(c->pid == tid){
      return c;
    }
    head = list_next(head);
  }
  return NULL;
}
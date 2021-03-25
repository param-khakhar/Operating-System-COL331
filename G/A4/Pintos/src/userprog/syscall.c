#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "userprog/exception.h"
#include "threads/malloc.h"
#include "vm/sup_page.h"
#include "vm/frame.h"

static void syscall_handler (struct intr_frame *);
bool lock_flag = true;

int sys_open(const char* name);
int sys_write(int fd, void* buffer, unsigned size);
void sys_exit(int status);
struct file* getFile(int fileD);
int insert_file(struct file* f);
void check(void* sp, int desired);
void check_str(void* str);
void check_buffer(const void* buffer, unsigned size);
void* convert(void* loc);
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

  printf("System calls not implemented correctly.\n");

  void* stack_pointer;
  int syscall_number;
  syscall_number = *(int *)convert(f->esp);

  switch(syscall_number){

    case SYS_CREATE:

      printf("Create System Call\n");
      break;

    case SYS_REMOVE:

      printf("Remove System Call\n");
      break;

    case SYS_OPEN:

      printf("Open System Call\n");
      check(stack_pointer, 1);
      void* arg = convert((stack_pointer+1));      
      f->eax = sys_open((const char*) arg);
      break;

    case SYS_CLOSE:

      printf("Close System Call\n");
      break;

    case SYS_READ:

      printf("Read System Call\n");
      break;

    case SYS_WRITE:

      printf("Write System Call\n");
      check(f->esp, 3);
      struct write_args* args = (struct write_args *)f->esp;
      check_buffer(args->buffer, args->size);
      f->eax = sys_write(args->fd, args->buffer, args->size);
      break;

    case SYS_SEEK:

      printf("Seek System Call\n");
      break;

    case SYS_FILESIZE:

      printf("FileSize system Call\n");
      break;

    case SYS_HALT:

      printf("Halt System Call\n");
      break;
    
    default:
      printf("Default!\n");
      break;
  }
  thread_exit();
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

int sys_write(int fd, void* buffer, unsigned size){

  if(size > 0){
    if(fd == STDOUT){
      putbuf(buffer, size);
      return size;
    }
    else{
      lock_acquire(&file_lock);
      struct file* fileptr = getFile(fd);
      if(!fileptr){
        lock_release(&file_lock);
        sys_exit(ERROR);
      }
      int written = file_write(fileptr, buffer, size);
      lock_release(&file_lock);
      return written;
    }
  }
  return size;  
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
  for(int i=0;i<desired;i++){
    addr = (int*)sp + i + 1;
    valid_address((const void*)addr);
  }
}

void check_str(void* str){
  char* curr = str;
  while(!convert((curr))){
    curr = (char*)curr +1;
  }
}

void check_buffer(const void* buffer, unsigned size){
  char* curr = (char*)buffer;
  for(unsigned i=0;i<size;i++){
    valid_address((const void*)curr);
    curr++; 
  }
}

void* convert(void* loc){
  void* kernel_addr = pagedir_get_page(thread_current()->pagedir, (loc));
  if(!kernel_addr){
    sys_exit(ERROR);
  }
  return kernel_addr;
}

void valid_address(const void* addr){
  if(!is_user_vaddr(addr)){
    sys_exit(ERROR);
  }
}

void sys_exit(int status){
  printf("Exiting with Status: %d\n",status);
  thread_exit();
}
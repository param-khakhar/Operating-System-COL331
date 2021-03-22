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
#include "vm/sup_page.h"
#include "vm/frame.h"

static void syscall_handler (struct intr_frame *);

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
      printf("System calls not implemented.\n");
      thread_exit();
}

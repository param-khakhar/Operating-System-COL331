#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "userprog/syscall.h"
#ifdef VM
#include "vm/frame.h"
#include "vm/sup_page.h"
#endif

#define MAX_ARGS (32)
static thread_func start_process NO_RETURN;
static bool load (const char *cmdline, void (**eip) (void), void **esp);

/* Starts a new thread running a user program loaded from
   FILENAME.  The new thread may be scheduled (and may even exit)
   before process_execute() returns.  Returns the new process's
   thread id, or TID_ERROR if the thread cannot be created. */
tid_t
process_execute (const char *file_name) 
{

  char *fn_copy;
  tid_t tid;

  if(strlen(file_name)+1>PGSIZE)
    return TID_ERROR;

  /* Make a copy of FILE_NAME.
     Otherwise there's a race between the caller and load(). */
  fn_copy = palloc_get_page (0);
  if (fn_copy == NULL)
    return TID_ERROR;

  strlcpy (fn_copy, file_name, PGSIZE);

  struct sync_aux* aux = (struct sync_aux*)malloc(sizeof(struct sync_aux));
  sema_init(&aux->sem, 0);
  aux->fn_clone = fn_copy;
  aux->cmd_line = file_name;

  /* Create a new thread to execute FILE_NAME. */
  tid = thread_create (file_name, PRI_DEFAULT, start_process, aux);
  sema_down(&aux->sem);
  // free memory on error
  if(tid == TID_ERROR) {
    palloc_free_page (fn_copy);
  }

  return tid;
}


/* A thread function that loads a user process and starts it
   running. */
static void
start_process (void *load_p)
{
  struct sync_aux* aux = (struct sync_aux *)load_p;
  char * file_name = aux->fn_clone;
  struct intr_frame if_;
  bool success;
  char *vargs[MAX_ARGS];
  int nargs = 0;
  int argssize = strlen(file_name) + 1;
  int debug = 0;

  /* tokenize command line */
  // char *token, *save_ptr;
  // for (token = strtok_r (file_name, " ", &save_ptr); token != NULL;
  //       token = strtok_r (NULL, " ", &save_ptr)) {
  //  vargs[nargs++] = token;
  //  ASSERT(nargs < MAX_ARGS);
  // }

  /* Initialize interrupt frame and load executable. */
  memset (&if_, 0, sizeof if_);
  if_.gs = if_.fs = if_.es = if_.ds = if_.ss = SEL_UDSEG;
  if_.cs = SEL_UCSEG;
  if_.eflags = FLAG_IF | FLAG_MBS;
  success = load (file_name, &if_.eip, &if_.esp);
  
  /* If load failed, quit. */
  sema_up(&aux->sem);
  if (!success){
    thread_exit ();
  }
  
    palloc_free_page (file_name);

  /* Start the user process by simulating a return from an
     interrupt, implemented by intr_exit (in
     threads/intr-stubs.S).  Because intr_exit takes all of its
     arguments on the stack in the form of a `struct intr_frame',
     we just point the stack pointer (%esp) to our stack frame
     and jump to it. */
  asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (&if_) : "memory");
  NOT_REACHED ();
}

/* Waits for thread TID to die and returns its exit status.  If
   it was terminated by the kernel (i.e. killed due to an
   exception), returns -1.  If TID is invalid or if it was not a
   child of the calling process, or if process_wait() has already
   been successfully called for the given TID, returns -1
   immediately, without waiting.

   This function will be implemented in problem 2-2.  For now, it
   does nothing. */
int
process_wait (tid_t child_tid)
{
  /*Find child_process from the list, and busy wait on the value of exit*/
  // printf("Wait for: %d, Parent(current): %d\n",child_tid, thread_tid());
  struct childProcess* cp = searchChild(child_tid);
  if(!cp)
    return ERROR;
  if(cp->waiting == 1)
    return ERROR;
  cp->waiting = 1;

  if(cp->exit_status == 0){
    /*Use a semaphore instead of busy waiting!*/
    // printf("CPID: %d\n",cp->pid);
    sema_down(&cp->waitLock);
  }
  int status = cp->status;
  // printf("StatusW: %d, Tid: %d\n",status,thread_tid());
  removeChild(child_tid, false);
  return status;
}

/* Free the current process's resources. */
void
process_exit (void)
{
  struct thread *cur = thread_current ();
  uint32_t *pd;

  sys_close(0, true);

  removeChild(0, true);

  if(findThread(cur->parent)){
    cur->corresp->exit_status = 1; 
  }

#ifdef VM
  /* close also the exec file assoiated with this */
  if(cur->execfile)
    file_close(cur->execfile);
#endif

  /* Destroy the current process's page directory and switch back
     to the kernel-only page directory. */
  pd = cur->pagedir;
  if (pd != NULL) 
    {
      /* Correct ordering here is crucial.  We must set
         cur->pagedir to NULL before switching page directories,
         so that a timer interrupt can't switch back to the
         process page directory.  We must activate the base page
         directory before destroying the process's page
         directory, or our active page directory will be one
         that's been freed (and cleared). */
      cur->pagedir = NULL;
      pagedir_activate (NULL);
      pagedir_destroy (pd);
    }
}

/* Sets up the CPU for running user code in the current
   thread.
   This function is called on every context switch. */
void
process_activate (void)
{
  struct thread *t = thread_current ();

  /* Activate thread's page tables. */
  pagedir_activate (t->pagedir);

  /* Set thread's kernel stack for use in processing
     interrupts. */
  tss_update ();
}

/* We load ELF binaries.  The following definitions are taken
   from the ELF specification, [ELF1], more-or-less verbatim.  */

/* ELF types.  See [ELF1] 1-2. */
typedef uint32_t Elf32_Word, Elf32_Addr, Elf32_Off;
typedef uint16_t Elf32_Half;

/* For use with ELF types in printf(). */
#define PE32Wx PRIx32   /* Print Elf32_Word in hexadecimal. */
#define PE32Ax PRIx32   /* Print Elf32_Addr in hexadecimal. */
#define PE32Ox PRIx32   /* Print Elf32_Off in hexadecimal. */
#define PE32Hx PRIx16   /* Print Elf32_Half in hexadecimal. */

/* Executable header.  See [ELF1] 1-4 to 1-8.
   This appears at the very beginning of an ELF binary. */
struct Elf32_Ehdr
  {
    unsigned char e_ident[16];
    Elf32_Half    e_type;
    Elf32_Half    e_machine;
    Elf32_Word    e_version;
    Elf32_Addr    e_entry;
    Elf32_Off     e_phoff;
    Elf32_Off     e_shoff;
    Elf32_Word    e_flags;
    Elf32_Half    e_ehsize;
    Elf32_Half    e_phentsize;
    Elf32_Half    e_phnum;
    Elf32_Half    e_shentsize;
    Elf32_Half    e_shnum;
    Elf32_Half    e_shstrndx;
  };

/* Program header.  See [ELF1] 2-2 to 2-4.
   There are e_phnum of these, starting at file offset e_phoff
   (see [ELF1] 1-6). */
struct Elf32_Phdr
  {
    Elf32_Word p_type;
    Elf32_Off  p_offset;
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
  };

/* Values for p_type.  See [ELF1] 2-3. */
#define PT_NULL    0            /* Ignore. */
#define PT_LOAD    1            /* Loadable segment. */
#define PT_DYNAMIC 2            /* Dynamic linking info. */
#define PT_INTERP  3            /* Name of dynamic loader. */
#define PT_NOTE    4            /* Auxiliary info. */
#define PT_SHLIB   5            /* Reserved. */
#define PT_PHDR    6            /* Program header table. */
#define PT_STACK   0x6474e551   /* Stack segment. */

/* Flags for p_flags.  See [ELF3] 2-3 and 2-4. */
#define PF_X 1          /* Executable. */
#define PF_W 2          /* Writable. */
#define PF_R 4          /* Readable. */

static bool setup_stack (void **esp, const char* cmd_line);
static bool validate_segment (const struct Elf32_Phdr *, struct file *);
static bool load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);
#ifdef VM
static bool lazy_load_segment (struct file *file, off_t ofs, uint8_t *upage,
                          uint32_t read_bytes, uint32_t zero_bytes,
                          bool writable);
#endif

/* Loads an ELF executable from FILE_NAME into the current thread.
   Stores the executable's entry point into *EIP
   and its initial stack pointer into *ESP.
   Returns true if successful, false otherwise. */
bool
load (const char *file_name, void (**eip) (void), void **esp) 
{
  struct thread *t = thread_current ();
  struct Elf32_Ehdr ehdr;
  struct file *file = NULL;
  off_t file_ofs;
  bool success = false;
  int i;

  /* Allocate and activate page directory. */
  t->pagedir = pagedir_create ();
  if (t->pagedir == NULL) 
    goto done;
  process_activate ();

  char* saveptr;
  char* split = malloc(strlen(file_name)+1);
  strlcpy(split, file_name, strlen(file_name)+1);

  split = strtok_r(split," ",&saveptr);
  /* Open executable file. */
  file = filesys_open (split);
  if (file == NULL) 
    {
      printf ("load: %s: open failed\n", file_name);
      goto done; 
    }
#ifdef VM
  t->execfile = file; /* store this for use in pagefault */
#endif
  /* Read and verify executable header. */
  if (file_read (file, &ehdr, sizeof ehdr) != sizeof ehdr
      || memcmp (ehdr.e_ident, "\177ELF\1\1\1", 7)
      || ehdr.e_type != 2
      || ehdr.e_machine != 3
      || ehdr.e_version != 1
      || ehdr.e_phentsize != sizeof (struct Elf32_Phdr)
      || ehdr.e_phnum > 1024) 
    {
      printf ("load: %s: error loading executable\n", file_name);
      goto done; 
    }

  /* Read program headers. */
  file_ofs = ehdr.e_phoff;
  for (i = 0; i < ehdr.e_phnum; i++) 
    {
      struct Elf32_Phdr phdr;

      if (file_ofs < 0 || file_ofs > file_length (file))
        goto done;
      file_seek (file, file_ofs);

      if (file_read (file, &phdr, sizeof phdr) != sizeof phdr)
        goto done;
      file_ofs += sizeof phdr;
      switch (phdr.p_type) 
        {
        case PT_NULL:
        case PT_NOTE:
        case PT_PHDR:
        case PT_STACK:
        default:
          /* Ignore this segment. */
          break;
        case PT_DYNAMIC:
        case PT_INTERP:
        case PT_SHLIB:
          goto done;
        case PT_LOAD:
          if (validate_segment (&phdr, file)) 
            {
              bool writable = (phdr.p_flags & PF_W) != 0;
              uint32_t file_page = phdr.p_offset & ~PGMASK;
              uint32_t mem_page = phdr.p_vaddr & ~PGMASK;
              uint32_t page_offset = phdr.p_vaddr & PGMASK;
              uint32_t read_bytes, zero_bytes;
              if (phdr.p_filesz > 0)
                {
                  /* Normal segment.
                     Read initial part from disk and zero the rest. */
                  read_bytes = page_offset + phdr.p_filesz;
                  zero_bytes = (ROUND_UP (page_offset + phdr.p_memsz, PGSIZE)
                                - read_bytes);
                }
              else 
                {
                  /* Entirely zero.
                     Don't read anything from disk. */
                  read_bytes = 0;
                  zero_bytes = ROUND_UP (page_offset + phdr.p_memsz, PGSIZE);
                }
#ifdef VM
              if (!lazy_load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
#else
              if (!load_segment (file, file_page, (void *) mem_page,
                                 read_bytes, zero_bytes, writable))
                goto done;
#endif
            }
          else
            goto done;
          break;
        }
    }

  /* Set up stack. */
  if (!setup_stack (esp, file_name))
    goto done;

  /* Start address. */
  *eip = (void (*) (void)) ehdr.e_entry;

  success = true;

 done:
  /* We arrive here whether the load is successful or not. */
/* Do not close this anymore - file will be used in exception.c page fault */
//  file_close (file);
  // thread_current()->corresp->exit_status = 1;
  if(!success)
    thread_current()->corresp->status = -1;
  else
    thread_current()->corresp->load_status = 1;
  return success;
}

/* load() helpers. */

// static bool install_page (void *upage, void *kpage, bool writable);

/* Checks whether PHDR describes a valid, loadable segment in
   FILE and returns true if so, false otherwise. */
static bool
validate_segment (const struct Elf32_Phdr *phdr, struct file *file) 
{
  /* p_offset and p_vaddr must have the same page offset. */
  if ((phdr->p_offset & PGMASK) != (phdr->p_vaddr & PGMASK)) 
    return false; 

  /* p_offset must point within FILE. */
  if (phdr->p_offset > (Elf32_Off) file_length (file)) 
    return false;

  /* p_memsz must be at least as big as p_filesz. */
  if (phdr->p_memsz < phdr->p_filesz) 
    return false; 

  /* The segment must not be empty. */
  if (phdr->p_memsz == 0)
    return false;
  
  /* The virtual memory region must both start and end within the
     user address space range. */
  if (!is_user_vaddr ((void *) phdr->p_vaddr))
    return false;
  if (!is_user_vaddr ((void *) (phdr->p_vaddr + phdr->p_memsz)))
    return false;

  /* The region cannot "wrap around" across the kernel virtual
     address space. */
  if (phdr->p_vaddr + phdr->p_memsz < phdr->p_vaddr)
    return false;

  /* Disallow mapping page 0.
     Not only is it a bad idea to map page 0, but if we allowed
     it then user code that passed a null pointer to system calls
     could quite likely panic the kernel by way of null pointer
     assertions in memcpy(), etc. */
  if (phdr->p_vaddr < PGSIZE)
    return false;

  /* It's okay. */
  return true;
}

/* Loads a segment starting at offset OFS in FILE at address
   UPAGE.  In total, READ_BYTES + ZERO_BYTES bytes of virtual
   memory are initialized, as follows:

        - READ_BYTES bytes at UPAGE must be read from FILE
          starting at offset OFS.

        - ZERO_BYTES bytes at UPAGE + READ_BYTES must be zeroed.

   The pages initialized by this function must be writable by the
   user process if WRITABLE is true, read-only otherwise.

   Return true if successful, false if a memory allocation error
   or disk read error occurs. */
static bool
load_segment (struct file *file, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) 
{
  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  file_seek (file, ofs);
  while (read_bytes > 0 || zero_bytes > 0) 
    {
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      /* Get a page of memory. */
      uint8_t *kpage = palloc_get_page (PAL_USER);
      if (kpage == NULL)
        return false;

      /* Load this page. */
      if (file_read (file, kpage, page_read_bytes) != (int) page_read_bytes)
        { 
          palloc_free_page (kpage);
          return false; 
        }
      memset (kpage + page_read_bytes, 0, page_zero_bytes);

      /* Add the page to the process's address space. */
      if (!install_page (upage, kpage, writable)) 
        { 
          palloc_free_page (kpage);
          return false; 
        }

      /* Advance. */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
    }
  return true;
}

#ifdef VM
static bool
lazy_load_segment (struct file *file UNUSED, off_t ofs, uint8_t *upage,
              uint32_t read_bytes, uint32_t zero_bytes, bool writable) {

  ASSERT ((read_bytes + zero_bytes) % PGSIZE == 0);
  ASSERT (pg_ofs (upage) == 0);
  ASSERT (ofs % PGSIZE == 0);

  while (read_bytes > 0 || zero_bytes > 0) 
    {
      /* Calculate how to fill this page.
         We will read PAGE_READ_BYTES bytes from FILE
         and zero the final PAGE_ZERO_BYTES bytes. */
      size_t page_read_bytes = read_bytes < PGSIZE ? read_bytes : PGSIZE;
      size_t page_zero_bytes = PGSIZE - page_read_bytes;

      struct sup_page* spg =
         new_file_sup_page(ofs, page_read_bytes, upage, writable);
      if(spg == NULL) {
        // TODO: cleanup and return false 
        PANIC("Could not make suppl. page. Should clean up!");
      }
   /* Advance. */
      ofs += page_read_bytes; /* WARNING! do not forget to update offset! */
      read_bytes -= page_read_bytes;
      zero_bytes -= page_zero_bytes;
      upage += PGSIZE;
    }
  return true;
}
#endif

/* Create a minimal stack by mapping a zeroed page at the top of
   user virtual memory. */
static bool
setup_stack (void **esp, const char* cmd_line) 
{
  uint8_t *kpage;
  bool success = false;
  uint8_t *upage = ((uint8_t *) PHYS_BASE) - PGSIZE;

#ifdef VM
  kpage = frame_map(upage, PAL_USER | PAL_ZERO);
#else
  kpage = palloc_get_page (PAL_USER | PAL_ZERO);
#endif
  if (kpage != NULL) 
    {
      success = install_page (upage, kpage, true);
      if (success) {
        *esp = PHYS_BASE; //  - 12; // if no args are implemented
#ifdef VM
        /* also store this in the suppl_page table */
        struct sup_page* spg = new_zero_sup_page(upage);
        ASSERT(spg != NULL);
        spg->frame_no = ADDR_TO_PFNO(kpage);
//        printf("Mapped %p -> %p\n", upage, kpage);
#endif
      } else
#ifdef VM
        frame_free(kpage);
#else
        palloc_free_page (kpage);
#endif
    }
    char* argv[MAX_ARGS];
    int argc = 0;
    char *token, *saveptr;
    int current = 2;
    for (token = strtok_r ((char*)cmd_line, " ", &saveptr); token != NULL; token = strtok_r (NULL, " ", &saveptr)){
      argv[argc] = token;
      argc++;
    }
    int debug = 0;
    /*Save the tokens on the stack first*/

    int i = argc-1;
    while(i >= 0){
      *esp -= (strlen(argv[i]) + 1);
      debug += (strlen(argv[i])+1);
      memcpy(*esp, argv[i], strlen(argv[i])+1);     //Copy the arguments themselves
      argv[i] = *esp;
      //printf("%s\n",*esp);
      //printf("Saved: %p\n",argv[i]);
      i--;
    }

    i = (PHYS_BASE - *esp) % 4;                       //Zeroing out the remaining locations, in case the address isn't
                                                      // word addressable.
    uint8_t zero = 0;
    if(i != 0){
      *esp -= (4-i);
      debug += (4-i);
      memcpy(*esp, &zero, (4-i)*sizeof(uint8_t));
      //printf("Aligned: %p\n",*esp);
    }

    /*Push Null*/
    int Zero = 0;
    *esp -= sizeof(char*);
    debug += sizeof(char*);
    memcpy(*esp, &Zero, sizeof(char*));

    /* Now push the pointers to the tokens on the stack */
    void* temp, *temp1;
    temp = *esp;
    //printf("Address-1: %p %s\n",*esp, *(char*)(temp));
    // temp = *esp;
    // printf("Address-1: %p %p\n", *(char*)temp,*esp);
    i = argc-1;
    while(i >= 0){
      *esp -= sizeof(char*);
      debug += sizeof(char*);
      memcpy(*esp, &argv[i], sizeof(char*));
      temp1 = *esp;
      i--;
    }
    /*Now push argv itself on the stack*/
    token = *esp;
    *esp -= sizeof(char**);
    debug += sizeof(char**);
    memcpy(*esp, &token, sizeof(char**));
    //printf("Stack: %p %p\n",*esp);
    /*Finally the argc, number of command line arguments*/

    *esp -= sizeof(int);
    debug += sizeof(int);
    memcpy(*esp, &argc, sizeof(int));
    //printf("Stack: %p\n",*esp);
    /*The call to main would exit, but conventionally, push the fake return address*/

    *esp -= sizeof(int);
    debug += sizeof(int);
    memcpy(*esp, &Zero, sizeof(int));
    //printf("Stack: %p\n",*esp);
    //hex_dump(0, *esp, debug, 1);


    // //free(argv);
  
  return success; 
}

/* Adds a mapping from user virtual address UPAGE to kernel
   virtual address KPAGE to the page table.
   If WRITABLE is true, the user process may modify the page;
   otherwise, it is read-only.
   UPAGE must not already be mapped.
   KPAGE should probably be a page obtained from the user pool
   with palloc_get_page().
   Returns true on success, false if UPAGE is already mapped or
   if memory allocation fails. */
bool
install_page (void *upage, void *kpage, bool writable)
{
  struct thread *t = thread_current ();

  /* Verify that there's not already a page at that virtual
     address, then map our page there. */
  return (pagedir_get_page (t->pagedir, upage) == NULL
          && pagedir_set_page (t->pagedir, upage, kpage, writable));
}

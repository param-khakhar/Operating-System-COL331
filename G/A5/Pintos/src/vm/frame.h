#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/palloc.h"

#define ADDR_TO_PFNO(a) (((unsigned)(a))>>12)
#define PFNO_TO_ADDR(n) ((void*)((n)<<12))

struct frame {
  struct thread* owner;
  unsigned kpage:20; /* frame number, physical address same as kernel address */
  unsigned upage:20; /* page number, virtual number same as user address */
  struct list_elem elem;
};

struct frame_table {
  struct list allframes;
  size_t count;
  struct lock mutex;
};

void frame_table_init(void);
void* frame_map(void* va, enum palloc_flags flags);
void frame_free(void* pa);

#endif /* VM_FRAME_H */

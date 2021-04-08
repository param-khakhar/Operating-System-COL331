#include "vm/frame.h"
#include "vm/sup_page.h"
#include "bitmap.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"

static struct frame_table ft;

/* helper */
static struct frame* find_frame_by_no(unsigned fn) {
  struct list_elem *e;
  for (e=list_begin(&ft.allframes); e!=list_end(&ft.allframes); e=list_next(e)) {
    struct frame *f = list_entry (e, struct frame, elem);
    if(f->kpage == fn) return f;
  }
  return NULL;
}

void frame_table_init(void) {
  ft.count = 0;
  list_init(&ft.allframes);
  lock_init(&ft.mutex);
}

void* frame_map(void* va, enum palloc_flags flags) {
  struct frame* f = NULL;
  lock_acquire(&ft.mutex);
  void* pa = palloc_get_page(flags);
  if(!pa) {
    /* no free frames. should evict */
    PANIC("No free frames!");
  } else {
    f = (struct frame*)malloc(sizeof(struct frame));
  }
  if(f) {
    f->kpage = ADDR_TO_PFNO(pa);
    f->upage = ADDR_TO_PFNO(va);
    f->owner = thread_current();
    list_push_back(&ft.allframes, &f->elem);
    ft.count++;
  }
  lock_release(&ft.mutex);
  return pa;
}

void frame_free(void* pa) {
  lock_acquire(&ft.mutex);
  struct frame* f=find_frame_by_no(ADDR_TO_PFNO(pa));
  if(f) { /* frame found */
    palloc_free_page(PFNO_TO_ADDR(f->kpage));
    list_remove(&f->elem);
    free(f);
  }
  lock_release(&ft.mutex);
}

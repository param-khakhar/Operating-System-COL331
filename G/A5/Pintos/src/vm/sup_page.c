#include "vm/sup_page.h"
#include "vm/frame.h"
#include <string.h>
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

/* helper, called by the below two to add a page to the thread list */
static struct sup_page* new_sup_page(enum page_location l, bool wr,  
                         unsigned pn, unsigned fn, 
                         /* struct file* pf in execfile,*/ off_t offs, uint32_t rd_b,
                         size_t bl_idx) {
  // does this need a lock? maybe for malloc only
  struct sup_page* pg = (struct sup_page*)malloc(sizeof(struct sup_page));
  if(pg != NULL) {
    *pg = (struct sup_page) {.location = l, .writable = wr, .page_no = pn, .frame_no = fn,
            .offset = offs, .read_bytes = rd_b};
    /* add it to the task page list */
    list_push_back(&thread_current()->sup_page_table, &pg->elem);
  }
  return pg;
}

/* Adds a new supplemental page to the current thread list of pages.
   This is linked to the file and offset used for loading the exec.
 */
struct sup_page* new_file_sup_page(off_t offs, uint32_t rd_b, 
                       uint8_t* upage, bool wr) {
  struct sup_page* pg = new_sup_page(DISK, wr, ADDR_TO_PFNO(upage),
                                     0, offs, rd_b, -1);
  /* builds info for an on DISK page. does not load anything,
     since page_fault should do it.
   */

  return pg;
}

/* Stack page entry only - must be zeroed and installed outside! 
   - for instance in setup or grow stack.
 */
struct sup_page* new_zero_sup_page(uint8_t* upage) {
  struct sup_page* pg = new_sup_page(MEMORY, true, ADDR_TO_PFNO(upage),
                                     0, 0, 0, -1);
  return pg;
}

/* Use to get information about a page during a page_fault */
struct sup_page* lookup_page(uint8_t* upage) {
  unsigned want_page_no = ADDR_TO_PFNO(upage);
  struct list* plst = &thread_current()->sup_page_table; 
  struct list_elem *e;
  for (e=list_begin(plst); e!=list_end(plst); e=list_next(e)) {
    struct sup_page *spg = list_entry (e, struct sup_page, elem);
    if(spg->page_no == want_page_no) return spg;
  }
  return NULL;
}

/* Frees the memory associated with the sup_page_table entries. 
   Should be called when processes finish.
 */
void sup_page_table_destroy(void) {
  struct list* plst = &thread_current()->sup_page_table; 
  while (!list_empty (plst)) {
    struct list_elem *e = list_pop_front (plst);
    struct sup_page *spg = list_entry (e, struct sup_page, elem);
    if(spg->location & MEMORY) { /* present in memory */
      /* frame_free is already done in the pagedir destroy! */
    }
    if(spg->location & DISK) { /* a file is open, close it */
      /* Should be done once. Done using execfile ptr in thread struct */
    }
    free(spg); 
  }
}

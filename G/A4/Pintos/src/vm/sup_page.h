#ifndef VM_SUP_PAGE_H
#define VM_SUP_PAGE_H

#include <list.h>
#include <stdlib.h>
#include "filesys/off_t.h"

/* Describes where pages are located. Allow for non-exclusivity -
   non-writable pages could reside on DISK and MEMORY at the same time.
   These should not be swapped out - they exist on DISK already.
 */
enum page_location {
  MEMORY = 1, /* resides in RAM */
  DISK = 2, /* in a file (executable) */
  SWAP = 4 /* on swap partition */
};

/* Used to keep track of where one page is. Every process/thread should have
   a list of these, to be used in a page_fault.
 */
struct sup_page {
  enum page_location location;
  bool writable; /* could be a code page, for instance */
  unsigned page_no;  /* most sig 20 bits describing the virtual addr */
  /* the following are non-exclusive */
  /* MEMORY */
  unsigned frame_no; /* most sig 20 bits describing kernel/phys addr */
  /* DISK - file is in thread execfile */
  off_t offset; /* offset in bytes */
  uint32_t read_bytes; /* number of bytes to read, up to PGSIZE */
  /* the rest will be zeroed */
  /* We'll do this as a list, but a hash table could be better */
  /* A hash table is already provided in "lib/kernel/hash.h,.c"*/
  struct list_elem elem;
};

struct sup_page* new_file_sup_page(off_t offs, uint32_t rd_b, 
                       uint8_t* upage, bool ro);
struct sup_page* new_zero_sup_page(uint8_t* upage);
struct sup_page* lookup_page(uint8_t* upage);
void sup_page_table_destroy(void);

#endif /* VM_SUP_PAGE_H */


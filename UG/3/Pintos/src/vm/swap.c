#include "vm/swap.h"
#include "bitmap.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "devices/block.h"

static struct block *swap_partition; // this might not need to be saved here
static struct bitmap *swap_freemap;
static struct lock swap_lock;

#define SECTORS_PER_PAGE (PGSIZE/BLOCK_SECTOR_SIZE)

/* this must be called at the right time */
void swap_init() {
  block_print_stats();  
  swap_partition = block_get_role(BLOCK_SWAP);
  if(swap_partition) {
    unsigned npgswp = block_size(swap_partition)/SECTORS_PER_PAGE;
    printf("Swap size is %d pages.\n",npgswp); 
    swap_freemap = bitmap_create(npgswp);
    bitmap_set_all(swap_freemap, true);
  }
  lock_init(&swap_lock);
}

/* returns the index of the swap page storing the given memory page */
size_t swap_out(const void* kpage) {
  size_t index = BITMAP_ERROR;
  lock_acquire(&swap_lock);
  index = bitmap_scan_and_flip(swap_freemap, 0, 1, true);
  if(index != BITMAP_ERROR) {
    block_sector_t swap_offset = index*SECTORS_PER_PAGE;
    for(block_sector_t i=0;i<SECTORS_PER_PAGE;i++) {
//      printf("swap out %p to %d\n", kpage+i*BLOCK_SECTOR_SIZE, swap_offset+i);
      block_write(swap_partition, swap_offset+i, kpage+i*BLOCK_SECTOR_SIZE);
    }
  }
  lock_release(&swap_lock);
  return index;
}

bool swap_in(size_t page_index, void* kpage) {
  bool ok = false;
  lock_acquire(&swap_lock);
  if(bitmap_contains(swap_freemap, page_index, 1, false)) {
    /* the swap page contains something (not free) */
    block_sector_t swap_offset = page_index*SECTORS_PER_PAGE;
    for(block_sector_t i=0;i<SECTORS_PER_PAGE;i++) {
//      printf("swap in %d to %p\n", swap_offset+i, kpage+i*BLOCK_SECTOR_SIZE);
      block_read(swap_partition, swap_offset+i, kpage+i*BLOCK_SECTOR_SIZE);
    }
    ok = true;
  }
  lock_release(&swap_lock);
  return ok;
}

void swap_free(size_t page_index) {
//  lock_acquire(&swap_lock);
  bitmap_mark(swap_freemap, page_index); // this is already atomic
//  lock_release(&swap_lock);
}

void swap_destroy() {
  ASSERT(swap_freemap);
  bitmap_destroy(swap_freemap);
}

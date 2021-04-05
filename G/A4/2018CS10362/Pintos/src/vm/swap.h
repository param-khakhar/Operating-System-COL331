#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <stdio.h>

void swap_init(void);
size_t swap_out(const void* kpage);
bool swap_in(size_t block_index, void* kpage);
void swap_free(size_t block_index);
void swap_destroy(void);

#endif /* VM_SWAP_H */

// based on cs3650 starter code

#ifndef PAGES_H
#define PAGES_H

#include <stdio.h>



// Get the number of blocks needed to store the given number of bytes.
int bytes_to_blocks(int bytes);

// Load and initialize the given disk image.
void blocks_init(const char* path);
// Close the disk image.
void blocks_free();
// Get the block with the given index, returning a pointer to its start.
void* blocks_get_block(int pnum);
// Return a pointer to the beginning of the block bitmap.
void* get_blocks_bitmap();
// Return a pointer to the beginning of the inode table bitmap.
void* get_inode_bitmap();
// Allocate a new block and return its index.
int alloc_block();
// Deallocate the block with the given index.
void free_block(int pnum);

#endif
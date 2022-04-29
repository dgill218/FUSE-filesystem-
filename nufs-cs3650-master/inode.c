#include <stdio.h>
#include "inode.h"
#include "blocks.h"
#include "bitmap.h"

// prints some stats about the inode
void print_inode(inode_t *node) {
    printf("inode_t located at %p:\n", node);
    printf("Node size: %d\n", node->size);
}

// Gets the inode from the given inum
inode_t * get_inode(int inum) {
    inode_t *inodes = get_inode_bitmap() + 32;
    return &inodes[inum];
}

// Allocaes a new inode
int alloc_inode() {
    int inode_num;
    for (int i = 0; i < 256; ++i) {
        if (!bitmap_get(get_inode_bitmap(), i)) {
            bitmap_put(get_inode_bitmap(), i, 1);
            inode_num = i;
            break;
        }
    }
    inode_t *new_node = get_inode(inode_num);
    new_node->refs = 1;
    new_node->size = 0;
    new_node->mode = 0;
    new_node->direct_pointers[0] = alloc_block();

    return inode_num;
}

// marks the inode_t as free in the bitmap and then clears the pointer locations
void free_inode(int inum) {
    shrink_inode(get_inode(inum), 0);
    free_block(get_inode(inum)->direct_pointers[0]);
    bitmap_put(get_inode_bitmap(), inum, 0);
}

// Increases the size of inode
// If too large, allocates a new page
int grow_inode(inode_t *node, int size) {
    int block_partition = node->size / 4096;
    for (int i = block_partition + 1; i <= size / 4096; i++) {
        // Direct ptrs
        if (i < num_ptrs) {
            node->direct_pointers[i] = alloc_block(); //alloc a page
        }
        else if (node->indirect_pointer == 0) {
            node->indirect_pointer = alloc_block();
            int *indirect_pointers = blocks_get_block(node->indirect_pointer); //retrieve memory loc.
            indirect_pointers[i - num_ptrs] = alloc_block(); //add another page
        }
        else {
            int *indirect_pointers = blocks_get_block(node->indirect_pointer); //retrieve memory loc.
            indirect_pointers[i - num_ptrs] = alloc_block(); //add another page
        }
    }
    node->size = size;
    return 0;
}

// shrinks an inode_t by the given size
int shrink_inode(inode_t *node, int size) {
    int block_partition = node->size / 4096;
    for (int i = block_partition; i > size / 4096; i--) {
        if (i < num_ptrs) { // direct pointers
            free_block(node->direct_pointers[i]); // free
            node->direct_pointers[i] = 0;
        } else if (i == num_ptrs) {
            free_block(node->indirect_pointer);
            node->indirect_pointer = 0;
        } else { // indirect pointers
            int *indirect_pointers = blocks_get_block(node->indirect_pointer);
            free_block(indirect_pointers[i - num_ptrs]); // free
            indirect_pointers[i - num_ptrs] = 0;
        }
    }
    node->size = size;
    return 0;
}

// gets the page number for the given inode
int inode_get_pnum(inode_t *node, int fpn) {
    int blockNum = fpn / 4096;
    if (blockNum < num_ptrs) {
        return node->direct_pointers[blockNum];
    } else {
        int *indirect_pointers = blocks_get_block(node->indirect_pointer);
        return indirect_pointers[blockNum - num_ptrs];
    }
}



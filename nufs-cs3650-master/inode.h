// based on cs3650 starter code

#ifndef INODE_H
#define INODE_H

#include "blocks.h"
#include <time.h>

#define nptrs 2

typedef struct inode {
    int refs; // reference count
    int mode; // permission & type
    int size; // bytes
    int ptrs[nptrs]; // direct pointers
    int iptr; // single indirect pointer
    time_t atim; // time last accessed
    time_t ctim; // time created
    time_t mtim; // time last modified
} inode_t;

void print_inode(inode_t* node);
inode_t* get_inode(int inum);
int alloc_inode();
void free_inode(int inum);
int grow_inode(inode_t* node, int size);
int shrink_inode(inode_t* node, int size);
int inode_get_pnum(inode_t* node, int fpn);
void decrease_refs(int inum);

#endif

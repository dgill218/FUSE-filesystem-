// based on cs3650 starter code

#ifndef DIRECTORY_H
#define DIRECTORY_H

#define DIR_NAME_LENGTH 48

#include "slist.h"
#include "blocks.h"
#include "inode.h"

typedef struct dirent {
    char name[DIR_NAME_LENGTH];// The name of the directory
    int  inum; // the number of the inode_t in the inode_t table?
    char used; // is this directory entry being used?
} dirent_t;

#define DIR_SIZE sizeof(dirent_t)

void directory_init();
int directory_lookup(inode_t *dd, const char *name);
int tree_lookup(const char *path);
int directory_put(inode_t *dd, const char *name, int inum);
int directory_delete(inode_t *dd, const char *name);
slist_t *directory_list(const char *path);
void print_directory(inode_t *dd);

#endif


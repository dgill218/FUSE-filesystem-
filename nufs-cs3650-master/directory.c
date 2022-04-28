// Implementation of directory.h

#include "slist.h"
#include "blocks.h"
#include "inode.h"
#include "directory.h"
#include "bitmap.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

// Initialized a directory, allocating an inode
void directory_init() {
    inode_t *root = get_inode(alloc_inode());
    root->mode = 040755;
}


int
directory_lookup(inode_t *dd, const char *name) {
    // Root directory
    if (!strcmp(name, "")) {
        return 0;
    } else {
        dirent_t *subdirs = blocks_get_block(dd->direct_pointers[0]);
        for (int i = 0; i < 64; ++i) {
            dirent_t cur = subdirs[i];
            if (strcmp(name, cur.name) == 0 && cur.used) {
                // Found directory
                return cur.inum;
            }
        }
        // Noting found :(
        return -1;
    }
}

int
tree_lookup(const char *path) {
    int curnode = 0;

    // parsing the path
    slist_t *pathlist = s_explode(path, '/');
    slist_t *currdir = pathlist;
    while (currdir != NULL) {
        // we look for the name of the next dir in the current one
        curnode = directory_lookup(get_inode(curnode), currdir->data);
        if (curnode == -1) {
            s_free(pathlist);
            return -1;
        }
        currdir = currdir->next;
    }
    s_free(pathlist);
    printf("tree lookup: %s is at node %d\n", path, curnode);
    return curnode;
}

// puts a new directory entry into the dir at dd that points to inode_t inum
int directory_put(inode_t *dd, const char *name, int inum) {
    int numberEntries = dd->size / sizeof(dirent_t);
    dirent_t *blockStart = blocks_get_block(dd->direct_pointers[0]);
    int beenAllocated = 0;

    dirent_t new;
    strncpy(new.name, name, DIR_NAME_LENGTH);
    new.inum = inum;
    new.used = 1;

    for (int i = 1; i < dd->size / sizeof(dirent_t); ++i) {
        if (blockStart[i].used == 0) {
            blockStart[i] = new;
            beenAllocated = 1;
        }
    }

    if (!beenAllocated) {
        blockStart[numberEntries] = new;
        dd->size = dd->size + sizeof(dirent_t);
    }
    return 0;
}

// this sets the matching directory to unused and takes a ref off its inode_t
int directory_delete(inode_t *dd, const char *name) {
    dirent_t *entries = blocks_get_block(dd->direct_pointers[0]);
    for (int i = 0; i < dd->size / sizeof(dirent_t); ++i) {
        if (strcmp(entries[i].name, name) == 0) {
            entries[i].used = 0;
            return 0;
        }
    }
    return -ENOENT;
}

// Gets a slist of directories at the given path
slist_t *directory_list(const char *path) {
    int current_dir = tree_lookup(path);
    inode_t *current_inode = get_inode(current_dir);

    int numdirs = current_inode->size / sizeof(dirent_t);
    dirent_t *dirs = blocks_get_block(current_inode->direct_pointers[0]);
    slist_t *dirnames = NULL;
    for (int i = 0; i < numdirs; ++i) {
        if (dirs[i].used) {
            dirnames = s_cons(dirs[i].name, dirnames);
        }
    }
    return dirnames;
}

// Prints the directory name.
void print_directory(inode_t *dd) {
    int dirCount = dd->size / sizeof(dirent_t);
    dirent_t *dirs = blocks_get_block(dd->direct_pointers[0]);
    for (int i = 0; i < dirCount; ++i) {
        printf("%s\n", dirs[i].name);
    }
}


#include "slist.h"
#include "blocks.h"
#include "inode.h"
#include "directory.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

// Initialize root.
void directory_init() {
    inode_t *root = get_inode(alloc_inode());
    root->mode = 040755;
}

// Finds the inum of the given inode with the given name.
int directory_lookup(inode_t *dd, const char *name) {
    dirent_t *lower_dirs = blocks_get_block(dd->direct_pointers[0]);
    for (int i = 0; i < 64; ++i) {
        dirent_t cur = lower_dirs[i];
        if (cur.used && strcmp(name, cur.name) == 0) {
            // Found directory
            return cur.inum;
        }
    }
    // Noting found :(
    return -1;
}

// Finds the node at the given path
int tree_lookup(const char *path) {
    int current_node = 0;

    // parsing the path
    slist_t *path_list = s_explode(path, '/');
    slist_t *current_directory = path_list;

    while (current_directory) {
        current_node = directory_lookup(get_inode(current_node), current_directory->data);
        if (current_node < 0) {
            s_free(path_list);
            return -1;
        }
        current_directory = current_directory->next;
    }
    s_free(path_list);
    return current_node;
}

// Makes new directory in the directory dd with the given inum
int directory_put(inode_t *dd, const char *name, int inum) {

    int number_entries = dd->size / DIR_SIZE;

    dirent_t *blockStart = blocks_get_block(dd->direct_pointers[0]);
    int beenAllocated = 0;

    dirent_t new;
    strncpy(new.name, name, DIR_NAME_LENGTH);
    new.inum = inum;
    new.used = 1;

    for (int i = 1; i < dd->size / DIR_SIZE; ++i) {
        if (blockStart[i].used == 0) {
            blockStart[i] = new;
            beenAllocated = 1;
        }
    }

    if (!beenAllocated) {
        blockStart[number_entries] = new;
        dd->size = dd->size + DIR_SIZE;
    }
    return 0;
}

// Deletes the given directory
int directory_delete(inode_t *dd, const char *name) {
    dirent_t *entries = blocks_get_block(dd->direct_pointers[0]);
    for (int i = 0; i < dd->size / DIR_SIZE; ++i) {
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

    int numdirs = current_inode->size / DIR_SIZE;
    dirent_t *dirs = blocks_get_block(current_inode->direct_pointers[0]);
    slist_t *dirnames = NULL;
    for (int i = 0; i < numdirs; ++i) {
        if (dirs[i].used) {
            dirnames = s_cons(dirs[i].name, dirnames);
        }
    }
    return dirnames;
}

// Prints all of the directory names from the given inode.
void print_directory(inode_t *dd) {
    int dirCount = dd->size / DIR_SIZE;
    dirent_t *dirs = blocks_get_block(dd->direct_pointers[0]);
    for (int i = 0; i < dirCount; ++i) {
        printf("%s\n", dirs[i].name);
    }
}

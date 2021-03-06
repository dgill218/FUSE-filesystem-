// Implementation of directory.h

#include "slist.h"
#include "blocks.h"
#include "inode.h"
#include "directory.h"
#include <string.h>
#include <stdio.h>


// Initialize root.
void directory_init() {
    inode_t *root = get_inode(alloc_inode());
    root->mode = 040755;
}

// Finds the inum of the given inode with the given name.
int directory_lookup(inode_t *dd, const char *name) {
    // Root directory
    if (!strcmp(name, "")) {
        return 0;
    } else {
        dirent_t *lower_directs = blocks_get_block(dd->direct_pointers[0]);
        for (int i = 0; i < 64; ++i) {
            dirent_t cur = lower_directs[i];
            if (strcmp(name, cur.name) == 0 && cur.used) {
                return cur.inum;
            }
        }
        // Noting found :(
        return -1;
    }
}

// Finds the node at the given path
int tree_lookup(const char *path) {
    int inum = 0;
    // parsing the path
    slist_t *path_list = s_explode(path, '/');
    while (path_list) {
        inum = directory_lookup(get_inode(inum), path_list->data);
        path_list = path_list->next;
    }
    return inum;
}

// Makes new directory in the directory dd with the given inum
int directory_put(inode_t *dd, const char *name, int inum) {

    int entry_count = dd->size / DIR_SIZE;

    dirent_t *blockStart = blocks_get_block(dd->direct_pointers[0]);
    int flag = 0;

    dirent_t dir;
    strncpy(dir.name, name, DIR_NAME_LENGTH);
    dir.inum = inum;
    dir.used = 1;

    for (int i = 1; i < entry_count; ++i) {
        if (blockStart[i].used == 0) {
            flag = 1;
            blockStart[i] = dir;
        }
    }
    if (!flag) {
        blockStart[entry_count] = dir;
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
    return -1;
}

// Gets a slist of directories at the given path
slist_t *directory_list(const char *path) {
    int current_dir = tree_lookup(path);
    inode_t *current_inode = get_inode(current_dir);

    int dir_count = current_inode->size / DIR_SIZE;
    dirent_t *dirs = blocks_get_block(current_inode->direct_pointers[0]);
    slist_t *list = NULL;
    for (int i = 0; i < dir_count; ++i) {
        if (dirs[i].used) {
            list = s_cons(dirs[i].name, list);
        }
    }
    return list;
}

// Prints the first directory name?
void print_directory(inode_t *dd) {
    dirent_t *dirs = blocks_get_block(dd->direct_pointers[0]);
    printf("%s", dirs[0].name);

}

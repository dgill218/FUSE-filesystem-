// Implementation of directory.h

#include "slist.h"
#include "blocks.h"
#include "inode.h"
#include "directory.h"
#include "bitmap.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*
typedef struct dirent_t {
    char name[DIR_NAME_LENGTH]; // The name of the directory
    int  inum; // the number of the inode_t in the inode_t table?
    char used; // is this used?
    char _reserved[11]; // rounding things out
} dirent_t; */


void directory_init() {
    // we already have our page for the inodes allocated
    // the root inode_t will be node 0
    inode_t* rootnode = get_inode(alloc_inode());
    rootnode->mode = 040755;
}

int 
directory_lookup(inode_t* dd, const char* name) {
    printf("looking for \"%s\" in dirlookup\n", name);
    if (!strcmp(name, "")) {
        printf("this is root, returning 0\n");
        return 0; 
    }
    dirent_t* subdirs = blocks_get_block(dd->dirPtrs[0]);
    for (int ii = 0; ii < 64; ++ii) {
        dirent_t csubdir = subdirs[ii];
        if (strcmp(name, csubdir.name) == 0 && csubdir.used) {
            printf("found a match! returning %d\n", csubdir.inum);
            return csubdir.inum;
        }
    }
    // if we can't find something that matches the name, return -1
    return -1;
}

// not sure if that's any different but imma use this to parse
int 
tree_lookup(const char* path) {
    int ii = 0;
    int jj = 0;
    int curnode = 0;
    
    // creates an slist_t with all the dir names and the file name
    slist_t* pathlist = s_explode(path, '/');
    slist_t* currdir = pathlist;
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
int directory_put(inode_t* dd, const char* name, int inum) {
    int numentries = dd->size / sizeof(dirent_t);
    dirent_t* entries = blocks_get_block(dd->dirPtrs[0]);
    int alloced = 0; // we want to keep track of whether we've alloced or not

    // building the new directory entry;
    dirent_t new;
    strncpy(new.name, name, DIR_NAME_LENGTH);
    new.inum = inum;
    new.used = 1;

    for (int ii = 1; ii < dd->size / sizeof(dirent_t); ++ii) {
        if (entries[ii].used == 0) {
            entries[ii] = new;
            alloced = 1;
        }
    }
    
    if (!alloced) {
        entries[numentries] = new;
        dd->size = dd->size + sizeof(dirent_t);
    }

    printf("running dir_put, putting %s, inum %d, on page %d\n", name, inum, dd->dirPtrs[0]);
    return 0;
}

// this sets the matching directory to unused and takes a ref off its inode_t
int directory_delete(inode_t* dd, const char* name) {
    printf("running dir delete on filename %s\n", name);
    dirent_t* entries = blocks_get_block(dd->dirPtrs[0]);
    printf("got direntries at block %d\n", dd->dirPtrs[0]);
    for (int ii = 0; ii < dd->size / sizeof(dirent_t); ++ii) {
        if (strcmp(entries[ii].name, name) == 0) {
            printf("found a deletion match at entry %d\n", ii);
            entries[ii].used = 0;
            decrease_refs(entries[ii].inum);
            return 0;
        }
    }
    printf("no file found! cannot delete");
    return -ENOENT;
}

// list of directories where? at the path? wait... this is for ls
slist_t* directory_list(const char* path) {
    int working_dir = tree_lookup(path);
    inode_t* w_inode = get_inode(working_dir);
    int numdirs = w_inode->size / sizeof(dirent_t);
    dirent_t* dirs = blocks_get_block(w_inode->dirPtrs[0]);
    slist_t* dirnames = NULL;
    for (int ii = 0; ii < numdirs; ++ii) {
        if (dirs[ii].used) {
            dirnames = s_cons(dirs[ii].name, dirnames);
        }
    }
    return dirnames;
}


void print_directory(inode_t* dd) {
    int numdirs = dd->size / sizeof(dirent_t);
    dirent_t* dirs = blocks_get_block(dd->dirPtrs[0]);
    for (int ii = 0; ii < numdirs; ++ii) {
        printf("%s\n", dirs[ii].name);
    }
}


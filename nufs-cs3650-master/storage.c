// implementation of storage.h

#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "slist.h"
#include "blocks.h"
#include "directory.h"
#include "inode.h"
#include "bitmap.h"
#include "util.h"

static void get_parent_child(const char* path, char* parent, char* child);

// initializes our file structure
void
storage_init(const char* path) {
    // initialize the pages
    blocks_init(path);
    // allocate a page for the inode_t list
    if (!bitmap_get(get_blocks_bitmap(), 1)) {
        for (int i = 0; i < 3; i++) {
            int newpage = alloc_block();
            printf("second inode_t page allocated at page %d\n", newpage);
        }
    }
    // the remaining pages will be alloced when we put data in them

    // then we initialize the root directory if it isn't allocated
    if (!bitmap_get(get_blocks_bitmap(), 4)) {
        printf("initializing root directory");
        directory_init();
    }
}

// check to see if the file is available, if not returns -ENOENT
int
storage_access(const char* path) {

    if (tree_lookup(path) >= 0) {
        return 0;
    }
    else
        return -ENOENT;
}

// mutates the stat with the inode_t features at the path
int 
storage_stat(const char* path, struct stat* st) {
    int working_inum = tree_lookup(path);
    if (working_inum > 0) {
        inode_t* node = get_inode(working_inum);
        st->st_mode = node->mode;
        st->st_size = node->size;
        st->st_nlink = node->refs;
        return 0;
    }
    return -1;
}

int 
storage_truncate(const char *path, off_t size) {
    int inum = tree_lookup(path);
    inode_t* node = get_inode(inum);
    if (node->size < size) {
	grow_inode(node, size);
    } else {
	shrink_inode(node, size);
    }
    return 0;
}

int 
storage_write(const char* path, const char* buf, size_t size, off_t offset)
{
    // get the start point with the path
    inode_t* write_node = get_inode(tree_lookup(path));
    if (write_node->size < size + offset) {
        storage_truncate(path, size + offset);
    }
    int bindex = 0;
    int nindex = offset;
    int rem = size;
    while (rem > 0) {
        char* dest = blocks_get_block(inode_get_pnum(write_node, nindex));
        dest += nindex % 4096;
        int cpyamnt = min(rem, 4096 - (nindex % 4096));
        memcpy(dest, buf + bindex, cpyamnt);
        bindex += cpyamnt;
        nindex += cpyamnt;
        rem -= cpyamnt;
    }
    return size;    
}

int
storage_read(const char* path, char* buf, size_t size, off_t offset)
{
    printf("storage_read called, buffer is\n%s\n", buf);
    inode_t* node = get_inode(tree_lookup(path));
    int bindex = 0;
    int nindex = offset;
    int rem = size;
    while (rem > 0) {
        char* src = blocks_get_block(inode_get_pnum(node, nindex));
        src += nindex % 4096;
        int cpyamnt = min(rem, 4096 - (nindex % 4096));
        memcpy(buf + bindex, src, cpyamnt);
        bindex += cpyamnt;
        nindex += cpyamnt;
        rem -= cpyamnt;
    }
    return size;
}

int
storage_mknod(const char* path, int mode) {
    // should add a direntry of the correct mode to the
    // directory at the path
        
    // check to make sure the node doesn't alreay exist
    if (tree_lookup(path) != -1) {
        return -EEXIST;
    }
 
    char* item = malloc(50);
    char* parent = malloc(strlen(path));
    get_parent_child(path, parent, item);

    int    pnodenum = tree_lookup(parent);
    if (pnodenum < 0) {
        free(item);
        free(parent);   
        return -ENOENT;
    }
    
    int new_inode = alloc_inode();
    inode_t* node = get_inode(new_inode);
    node->mode = mode;
    node->size = 0;
    node->refs = 1;
    inode_t* parent_dir = get_inode(pnodenum);

    directory_put(parent_dir, item, new_inode);
    free(item);
    free(parent);
    return 0;
}

// this is used for the removal of a link. If refs are 0, then we also
// delete the inode_t associated with the dirent_t
int
storage_unlink(const char* path) {
    char* nodename = malloc(50);
    char* parentpath = malloc(strlen(path));
    get_parent_child(path, parentpath, nodename);

    inode_t* parent = get_inode(tree_lookup(parentpath));
    int rv = directory_delete(parent, nodename);

    free(parentpath);
    free(nodename);

    return rv;
}

int    
storage_link(const char *from, const char *to) {
    int tnum = tree_lookup(to);
    if (tnum < 0) {
        return tnum;
    }

    char* fname = malloc(50);
    char* fparent = malloc(strlen(from));
    get_parent_child(from, fparent, fname);

    inode_t* pnode = get_inode(tree_lookup(fparent));
    directory_put(pnode, fname, tnum);
    get_inode(tnum)->refs ++;
    
    free(fname);
    free(fparent);
    return 0;
}

int    
storage_rename(const char *from, const char *to) {
    storage_link(to, from);
    storage_unlink(from);
    return 0;
}


int storage_set_time(const char* path, const struct timespec ts[2])
{
    int nodenum = tree_lookup(path);
    if (nodenum < 0) {
        return -ENOENT;
    }
    return 0;
}


slist_t* storage_list(const char* path) {
    return directory_list(path);
}

static void get_parent_child(const char* path, char* parent, char* child) {
    slist_t* flist = s_explode(path, '/');
    slist_t* fdir = flist;
    parent[0] = 0;
    while (fdir->next != NULL) {
        strncat(parent, "/", 1);
        strncat(parent, fdir->data, 48);
        fdir = fdir->next;
    }
    memcpy(child, fdir->data, strlen(fdir->data));
    child[strlen(fdir->data)] = 0;
    s_free(flist);
}

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

#define NAME_SIZE 50

// These are helper methods for storage_read and storage_write.
// They do the actual reading and writing from the buffers.
void write_help(int first_i, int second_i, int remainder, inode_t* node, const char* buf);
void read_help(int first_i, int second_i, int remainder, inode_t* node, const char* buf);

// initialize our basic file structure
void storage_init(const char* path) {
    blocks_init(path);
    // allocate a page for the inode_t list
    /*if (!bitmap_get(get_blocks_bitmap(), 1)) {
        for (int i = 0; i < 3; i++) {
            int newpage = alloc_block();
        }
    }*/

    if (!bitmap_get(get_blocks_bitmap(), 4)) {
        directory_init();
    }
}

// check to see if the file is available, if not returns -ENOENT
int storage_access(const char* path) {
    if (tree_lookup(path) >= 0) {
        return 0;
    }
    else
        return -1;
}

// Changes the stats to the file stats.
int storage_stat(const char* path, struct stat* st) {
    int working_inum = tree_lookup(path);
    if (working_inum > 0) {
        inode_t *node = get_inode(working_inum);
        st->st_mode = node->mode;
        st->st_size = node->size;
        st->st_nlink = node->refs;
        return 0;
    }
    else {
        return -1;
    }
}

// Truncates the file at the given path to the given size.
int storage_truncate(const char *path, off_t size) {
    int inum = tree_lookup(path);
    inode_t* node = get_inode(inum);
    if (node->size > size) {
        shrink_inode(node, size);
    }
    else {
        grow_inode(node, size);
    }
    return 0;
}

void write_help(int first_i, int second_i, int remainder, inode_t* node, const char* buf) {
    while (remainder > 0) {
        char* dest = blocks_get_block(inode_get_pnum(node, second_i));
        dest += second_i % 4096;
        int copy_amount = (remainder < 4096 - (second_i % 4096)) ? remainder : 4096 - (second_i % 4096);
        //int copy_amount = min(remainder, 4096 - (second_i % 4096));

        memcpy(dest, buf + first_i, copy_amount);
        first_i += copy_amount;
        second_i += copy_amount;
        remainder -= copy_amount;
    }
}

void read_help(int first_i, int second_i, int remainder, inode_t* node, const char* buf) {
    while (remainder > 0) {
        char* src = blocks_get_block(inode_get_pnum(node, second_i));
        src += second_i % 4096;
        // int copy_amount = min(remainder, 4096 - (second_i % 4096));
        int copy_amount = (remainder < 4096 - (second_i % 4096)) ? remainder : 4096 - (second_i % 4096);

        memcpy(buf + first_i, src, copy_amount);
        first_i += copy_amount;
        second_i += copy_amount;
        remainder -= copy_amount;
    }
}

// Writes to the path from the buf
int storage_write(const char* path, const char* buf, size_t size, off_t offset)
{

    inode_t* write_node = get_inode(tree_lookup(path));
    // Make sure size is valid
    if (write_node->size < size + offset) {
        storage_truncate(path, size + offset);
    }
    int first_i = 0;
    int second_i = offset;
    int remainder = size;
    write_help(first_i, second_i, remainder, write_node, buf);
    return size;
}



// Reads from the file at the given path
int storage_read(const char* path, char* buf, size_t size, off_t offset)
{
    inode_t* read_node = get_inode(tree_lookup(path));
    int first_i = 0;
    int second_i = offset;
    int remainder = size;
    read_help(first_i, second_i, remainder, read_node, buf);
    return size;
}


// Add a directory at the current path
int storage_mknod(const char* path, int mode) {
    if (tree_lookup(path) != -1) {
        return -1;
    }
    else {
        size_t path_len = strlen(path);
        char *item = malloc(NAME_SIZE);
        char *parent = malloc(path_len);

        slist_t *file_list = s_explode(path, '/');
        slist_t *dir_list = file_list;
        parent[0] = 0;
        while (dir_list->next) {

            strncat(parent, "/", 1);
            strncat(parent, dir_list->data, 48);
            dir_list = dir_list->next;
        }
        size_t dir_list_len = strlen(dir_list->data);
        memcpy(item, dir_list->data, dir_list_len);
        item[dir_list_len] = 0;
        s_free(file_list);
        int node_num = tree_lookup(parent);
        int new_inode = alloc_inode();
        inode_t *node = get_inode(new_inode);

        node->mode = mode;
        node->size = 0;
        node->refs = 1;

        directory_put(get_inode(node_num), item, new_inode);
        return 0;
    }
}

// Removes a link
int storage_unlink(const char* path) {
    char* name = malloc(NAME_SIZE);
    char* parent = malloc(strlen(path));

    slist_t* path_list = s_explode(path, '/');
    slist_t* temp = path_list;

    parent[0] = 0;
    while (temp->next != NULL) {
        strncat(parent, "/", 1);
        strncat(parent, temp->data, 48);
        temp = temp->next;
    }
    char* data = temp->data;
    memcpy(name, data, strlen(data));
    name[strlen(data)] = 0;
    s_free(path_list);

    inode_t* parent_node = get_inode(tree_lookup(parent));
    int rv = directory_delete(parent_node, name);

    return rv;
}

// Adds a link
int storage_link(const char *from, const char *to) {
    slist_t* path_list = s_explode(from, '/');
    slist_t* temp = path_list;

    char* name = malloc(NAME_SIZE);
    char* parent = malloc(strlen(from));

    parent[0] = 0;
    while (temp->next) {
        strncat(parent, "/", 1);
        strncat(parent, temp->data, 48);
        temp = temp->next;
    }
    char* data = temp->data;
    memcpy(name, data, strlen(data));
    name[strlen(data)] = 0;
    s_free(path_list);

    inode_t* parent_node = get_inode(tree_lookup(parent));
    directory_put(parent_node, name, tree_lookup(to));
    get_inode(tree_lookup(to))->refs += 1;

    return 0;
}

// Renames the file at the from path to the to path.
int storage_rename(const char *from, const char *to) {
    storage_link(to, from);
    storage_unlink(from);
    return 0;
}

// Sets the times?
int storage_set_time(const char* path, const struct timespec ts[2])
{
    int nodenum = tree_lookup(path);
    if (nodenum < 0) {
        return -1;
    }
    return 0;
}

// Lists the directories at the path
slist_t* storage_list(const char* path) {
    return directory_list(path);
}


// implementation of storage.h

#include <sys/stat.h>
#include <time.h>
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
void write_help(int first_i, int second_i, int remainder, inode_t *node, const char *buf);

void read_help(int first_i, int second_i, int remainder, inode_t *node, const char *buf);

// initialize our basic file structure
void storage_init(const char *path) {
    blocks_init(path);
    if (!bitmap_get(get_blocks_bitmap(), 4)) {
        directory_init();
    }
}

// check to see if the file is available, if not returns -1
int storage_access(const char *path) {
    if (tree_lookup(path) >= 0) {
        return 0;
    } else
        return -1;
}

// Changes the stats to the file stats.
int storage_stat(const char *path, struct stat *st) {
    int inum = tree_lookup(path);
    if (inum > 0) {
        inode_t *node = get_inode(inum);
        st->st_nlink = node->refs;
        st->st_mode = node->mode;
        st->st_size = node->size;
        return 0;
    } else {
        return -1;
    }
}

// Truncates the file at the given path to the given size.
int storage_truncate(const char *path, off_t size) {
    int inum = tree_lookup(path);
    inode_t *node = get_inode(inum);
    if (node->size > size) {
        shrink_inode(node, size);
    } else {
        grow_inode(node, size);
    }
    return 0;
}

void write_help(int first_i, int second_i, int remainder, inode_t *node, const char *buf) {
    while (1) {
        char *dest = blocks_get_block(inode_get_pnum(node, second_i));
        dest += second_i % 4096;
        int size;
        if (remainder < 4096 - (second_i % 4096)) {
            size = remainder;
        } else {
            size = 4096 - (second_i % 4096);
        }

        memcpy(dest, buf + first_i, size);
        first_i += size;
        second_i += size;
        remainder -= size;
        if (remainder <= 0) {
            break;
        }
    }
}

void read_help(int first_i, int second_i, int remainder, inode_t *node, const char *buf) {
    while (1) {
        char *src = blocks_get_block(inode_get_pnum(node, second_i));
        src += second_i % 4096;
        int size;
        if (remainder < 4096 - (second_i % 4096)) {
            size = remainder;
        } else {
            size = 4096 - (second_i % 4096);
        }
        memcpy(buf + first_i, src, size);
        first_i += size;
        second_i += size;
        remainder -= size;
        if (remainder <= 0) {
            break;
        }
    }
}

// Writes to the path from the buf. Returns the size of the data written
int storage_write(const char *path, const char *buf, size_t size, off_t offset) {
    inode_t *node = get_inode(tree_lookup(path));
    // Make sure size is valid
    if (node->size < size + offset) {
        storage_truncate(path, size + offset);
    }
    int first_i = 0;
    int second_i = offset;
    int remainder = size;
    write_help(first_i, second_i, remainder, node, buf);
    return size;
}


// Reads from the file at the given path. Returns the size of the data read.
int storage_read(const char *path, char *buf, size_t size, off_t offset) {
    inode_t *node = get_inode(tree_lookup(path));
    int first_i = 0;
    int second_i = offset;
    int remainder = size;
    read_help(first_i, second_i, remainder, node, buf);
    return size;
}


// Add a directory at the current path
int storage_mknod(const char *path, int mode) {
    size_t path_len = strlen(path);
    char *item = malloc(NAME_SIZE);
    char *parent = malloc(path_len);

    slist_t *file_list = s_explode(path, '/');
    slist_t *dir_list = file_list;

    parent[0] = 0;
    while (dir_list->next) {
        strncat(parent, "/", 1);
        strncat(parent, dir_list->data, DIR_NAME_LENGTH);
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

// Renames the file at the from path to the to path.
int storage_rename(const char *from, const char *to) {
    storage_link(to, from);
    storage_unlink(from);
    return 0;
}

// Sets the times? Not really sure why we need this, no tests on it
int storage_set_time(const char *path, const struct timespec ts[2]) {
    return 0;
}

// Lists the directories at the path
slist_t *storage_list(const char *path) {
    return directory_list(path);
}


// based on cs3650 starter code

#define _GNU_SOURCE
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>

#include "blocks.h"
#include "util.h"
#include "bitmap.h"

const int BLOCK_COUNT = 256;
const int BLOCK_SIZE = 4096;
const int NUFS_SIZE  = BLOCK_SIZE * 256; // 1MB

static int   pages_fd   = -1;
static void* pages_base =  0;

void
blocks_init(const char* path)
{
    pages_fd = open(path, O_CREAT | O_RDWR, 0644);
    assert(pages_fd != -1);

    int rv = ftruncate(pages_fd, NUFS_SIZE);
    assert(rv == 0);

    pages_base = mmap(0, NUFS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, pages_fd, 0);
    assert(pages_base != MAP_FAILED);

    void* pbm = get_blocks_bitmap();
    bitmap_put(pbm, 0, 1);
}

void
blocks_free()
{
    int rv = munmap(pages_base, NUFS_SIZE);
    assert(rv == 0);
}

void*
blocks_get_block(int pnum)
{
    return pages_base + 4096 * pnum;
}

void*
get_blocks_bitmap()
{
    return blocks_get_block(0);
}

void*
get_inode_bitmap()
{
    uint8_t* page = blocks_get_block(0);
    return (void*)(page + 32);
}

int
alloc_block()
{
    void* pbm = get_blocks_bitmap();

    for (int ii = 1; ii < BLOCK_COUNT; ++ii) {
        if (!bitmap_get(pbm, ii)) {
            bitmap_put(pbm, ii, 1);
            printf("+ alloc_page() -> %d\n", ii);
            return ii;
        }
    }

    return -1;
}

void
free_block(int pnum)
{
    printf("+ free_page(%d)\n", pnum);
    void* pbm = get_blocks_bitmap();
    bitmap_put(pbm, pnum, 0);
}

// Implementation of bitmap.h

#include "bitmap.h"
#include <stdio.h>
#include <stdint.h>

#define nth_bit_mask(n) (1 << (n))
#define byte_index(n) ((n) / 8)
#define bit_index(n) ((n) % 8)

// gets the value at but ii
int bitmap_get(void* bm, int i) {
    uint8_t *base = (uint8_t *) bm;

    return (base[byte_index(i)] >> bit_index(i)) & 1;
}

// puts a value at the specified bit in the bitmap
void bitmap_put(void* bm, int i, int v) {
    uint8_t *base = (uint8_t *) bm;

    long bit_mask = nth_bit_mask(bit_index(i));

    if (v) {
        base[byte_index(i)] |= bit_mask;
    } else {
        bit_mask = ~bit_mask;
        base[byte_index(i)] &= bit_mask;
    }
}

// debug statement
void bitmap_print(void* bm, int size) {
    for (int i = 0; i < size; i++) {
        putchar(bitmap_get(bm, i) ? '1' : '0');

        if ((i + 1) % 64 == 0) {
            putchar('\n');
        } else if ((i + 1) % 8 == 0) {
            putchar(' ');
        }
    }
}

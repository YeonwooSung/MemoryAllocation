#ifndef SUBSET_H
#define SUBSET_H

/**
 * A header (offset) of the allocated memory block.
 */
typedef struct header {
    char free : 1;
    unsigned char hasPrev : 1;
    unsigned char hasNext : 1;
    int size;
} Header;

/**
 * A linked list of free memory blocks.
 */
typedef struct list {
    struct list *prev; //pointer that points the previous block.
    struct list *next; //pointer that points the next block.
} FreeList;

#endif
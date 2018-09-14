#include <stdio.h>

#include "mymalloc.h"
#include "subset.h"

//boolean
#define FALSE 0
#define TRUE 1
#define USED FALSE
#define FREE TRUE

//the linked list of free memory blocks.
FreeList *freeList = NULL;

//to reduce the number of mmap calls.
int lastMappedSize; //TODO choose the initial value of the lastMappedSize.

//Offset size
#define OFFSET_SIZE sizeof(Header)

/**
 * The aim of this function is to allocate the memory dynamically.
 *
 * @param size the size of the memory
 * @return ptr the starting point of the allocated memory.
 */
void *myalloc(int size) {
    //returns NULL if the size is less than or equal to 0.
    if (size <= 0) {
        return NULL;
    }

    /*
     * The size of the assigned memory block could not be smaller than the size of the offset, as long as
     * the memory block contains the offset (header) in it.
     */
    int requiredSize = size  + OFFSET_SIZE;

    return NULL;
}

/**
 * This function frees the dynamically allocated memory.
 *
 * @param ptr the pointer that points the starting point of the address that should be freed.
 */
void myfree(void *ptr) {
    //
}
#include <stdio.h>

#include "mymalloc.h"
#include "subset.h"

#define FALSE 0
#define TRUE 1
#define USED FALSE
#define FREE TRUE

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
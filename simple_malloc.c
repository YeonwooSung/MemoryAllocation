#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "simple_malloc.h"

/**
 * A simple implementation of memory allocating function.
 *
 * @param size the size of the memory that should be allocated.
 * @return If the sbrk failed, returns NULL. Otherwise, returns p, which is the starting point of the allocated memory.
 */
void *simpleMalloc(size_t size) {
    void *p = sbrk(0);
    void *request = sbrk(size);

    if (request == (void *)-1) {
        return NULL; //sbrk failed
    } else {
        assert(p == request); //Not thread safe.
        return p;
    }
}
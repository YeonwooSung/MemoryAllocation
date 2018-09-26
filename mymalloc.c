#include <stdio.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <math.h>
#include <stdint.h>

#include "mymalloc.h"

int *last_address = 0;

#define FALSE 0
#define TRUE 1
#define USED FALSE
#define FREE TRUE

typedef struct Header {
    int size;
    unsigned int free : 1;
    unsigned int has_prev : 1;
    unsigned int has_next : 1;
} Header;

// Size field is not necessary in used blocks.
typedef struct Footer {
    int size;
    unsigned int free : 1;
} Footer;

// Size 16
typedef struct free_list {
    struct free_list *next;
    struct free_list *prev;
} free_list;

free_list *free_list_start = NULL;

// To reduce number of mmap calls.
int last_mapped_size = 1;

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define PAGE_SIZE sysconf(_SC_PAGESIZE)
#define CEIL(X) ((X - (int)(X)) > 0 ? (int)(X + 1) : (int)(X))
#define PAGES(size) (CEIL(size / (double)PAGE_SIZE))
#define MIN_SIZE (ALIGN(sizeof(free_list) + META_SIZE))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

// Meta sizes.
#define META_SIZE ALIGN(sizeof(Header) + sizeof(Footer))
#define HEADER_SIZE ALIGN(sizeof(Header))
#define FOOTER_SIZE ALIGN(sizeof(Footer))

// Get pointer to the payload (passing the pointer to the header).
void *add_offset(void *ptr) {
    return ptr + HEADER_SIZE;
}

// Get poiner to the header (passing pointer to the payload).
void *remove_offset(void *ptr) {
    return ptr - HEADER_SIZE;
}

void *getFooter(void *header_ptr) {
    return header_ptr + ((Header *)header_ptr)->size - FOOTER_SIZE;
}

void setFree(void *ptr, int val) {
    ((Header *)ptr)->free = val;
    Footer *footer = (Footer *)getFooter(ptr);
    footer->free = val;
    // Copy size to footer size field.
    footer->size = ((Header *)ptr)->size;
}

// Set size in the header.
void setSizeHeader(void *ptr, int size) {
    ((Header *)ptr)->size = size;
}

// Set size in the header.
void setSizeFooter(void *ptr, int size) {
    ((Footer *)getFooter(ptr))->size = size;
}

// Get size of the free list item.
int getSize(void *ptr) {
    return ((Header *)remove_offset(ptr))->size;
}

void remove_from_free_list(void *block) {
    setFree(block, USED);

    free_list *free_block = (free_list *)add_offset(block);
    free_list *next = free_block->next;
    free_list *prev = free_block->prev;
    if (!prev) {
        if (!next) {
            // free_block is the only block in the free list.
            free_list_start = NULL;
        } else {
            // Remove first element in the free list.
            free_list_start = next;
            next->prev = NULL;
        }
    } else {
        if (!next) {
            // Remove last element of the free list.
            prev->next = NULL;
        } else {
            // Remove element in the middle.
            prev->next = next;
            next->prev = prev;
        }
    }
}

void append_to_free_list(void *ptr) {
    setFree(ptr, FREE);

    free_list new = {};
    free_list *new_ptr = (free_list *)add_offset(ptr);
    *new_ptr = new;

    if (free_list_start) {
        // Insert in the beginning.
        new_ptr->next = free_list_start;
        new_ptr->prev = NULL;
        free_list_start->prev = new_ptr;
        free_list_start = new_ptr;
    } else {
        // No elements in the free list
        free_list_start = new_ptr;
        new_ptr->prev = NULL;
        new_ptr->next = NULL;
    }
}

// Find a free block that is large enough to store 'size' bytes.
// Returns NULL if not found.
free_list *find_free_block(int size) {
    free_list *current = free_list_start;
    while (current) {
        if (getSize(current) >= size) {
            // Return a pointer to the free block.
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Split memory into multiple blocks after some part of it was requested
// (requested + the rest).
void split(void *start_ptr, int total, int requested) {
    void *new_block_ptr = start_ptr + requested;
    int block_size = total - requested;

    // Size that was left after allocating memory.
    // Needs to be large enough to store another block (min size is needed in order
    // to store free list element there after it is freed).
    if (block_size < MIN_SIZE) {
        // Not enough size to split.
        return;
    }
    // Change size of the prev (recently allocated) block.
    setSizeHeader(start_ptr, requested);
    ((Header *)start_ptr)->has_next = TRUE;

    // Add a header for newly created block (right block).
    Header header = {block_size, FREE, TRUE, ((Header *)start_ptr)->has_next};
    Header *new_block_header = (Header *)new_block_ptr;
    *new_block_header = header;
    Footer footer = {block_size, FREE};
    *((Footer *)getFooter(new_block_header)) = footer;
    append_to_free_list(new_block_header);
}

void *myalloc(int size) {
    if (size <= 0) {
        return NULL;
    }
    // Size of the block can't be smaller than MIN_SIZE, as we need to store
    // free list in the body + header and footer on each side respectively.
    int required_size = MAX(ALIGN(size + META_SIZE), MIN_SIZE);
    // Try to find a block big enough in already allocated memory.
    free_list *free_block = find_free_block(required_size);

    if (free_block) {
        // Header ptr
        void *address = remove_offset(free_block);
        // Mark block as used.
        setFree(address, USED);
        // Split the block into two, where the second is free.
        split(address, ((Header *)address)->size, required_size);
        remove_from_free_list(address);
        return add_offset(address);
    }

    // No free block was found. Allocate size requested + header (in full pages).
    // Each next allocation will be doubled in size from the previous one
    // (to decrease the number of mmap sys calls we make).
    int bytes = MAX(PAGES(required_size), last_mapped_size) * PAGE_SIZE;
    last_mapped_size *= 2;

    // last_address my not be returned by mmap, but makes it more efficient if it happens.
    void *new_region = mmap(last_address, bytes, PROT_READ | PROT_WRITE,
                            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (new_region == MAP_FAILED) {
        return NULL;
    }
    // Create a header/footer for new block.
    Header header = {bytes, USED, FALSE, FALSE};
    Header *header_ptr = (Header *)new_region;
    *header_ptr = header;
    Footer footer = {};
    footer.free = USED;
    *((Footer *)getFooter(new_region)) = footer;

    if (new_region == last_address && last_address != 0) {
        // if we got a block of memory after the last block, as we requested.
        header_ptr->has_prev = TRUE;
        // change has_next of the prev block
        Footer *prev_footer = (Footer *)(header_ptr - FOOTER_SIZE);
        ((Header *)header_ptr - (prev_footer->size))->has_next = TRUE;
    }
    // Split new region.
    split(new_region, bytes, required_size);
    // Update last_address for the next allocation.
    last_address = new_region + bytes;
    // Return address behind the header (i.e. header is hidden).
    return add_offset(new_region);
}

void coalesce(void *ptr) {
    Header *current_header = (Header *)ptr;
    Footer *current_footer = (Footer *)getFooter(ptr);
    if (current_header->has_prev && ((Footer *)(ptr - FOOTER_SIZE))->free) {
        int prev_size = ((Footer *)(ptr - FOOTER_SIZE))->size;
        Header *prev_header = (Header *)(ptr - prev_size);
        Footer *prev_footer = (Footer *)((Footer *)(ptr - FOOTER_SIZE));

        // Merge with previous block.
        remove_from_free_list(current_header);
        // Add size of prev block to the size of current block
        prev_header->size += current_header->size;
        prev_footer->size = prev_header->size;
        current_header = prev_header;
    }
    void *next = ptr + current_header->size;
    if (current_header->has_next && ((Header *)next)->free) {
        int size = ((Header *)next)->size;
        // merge with next block.
        remove_from_free_list(ptr + current_header->size);
        // Add size of next block to the size of current block.
        current_header->size += size;
        current_footer->size = current_header->size;
    }
}

int unmap(void *start_address, int size) {
    remove_from_free_list(start_address);
    // Reset has_next, has_prev of neighbours.
    Header *header = (Header *)start_address;

    if (header->has_prev) {
        // Get prev header, set has_next to false.
        int prev_size = ((Footer *)(start_address - FOOTER_SIZE))->size;
        Header *prev_header = (Header *)(start_address - prev_size);
        prev_header->has_next = FALSE;

    } 
    
    if (header->has_next) {
        // Get next header, set has_prev to false.
        int this_size = header->size;
        Header *next_header = (Header *)(start_address + this_size);
        next_header->has_prev = FALSE;
    }

    // If this is the last block we've allocated using mmap, need to change last_address.
    if (last_address == start_address) {
        last_address = start_address - size;
    }

    return munmap(start_address, (size_t)size);
}

void myfree(void *ptr) {
    if (!ptr) {
        return;
    }

    void *start_address = remove_offset(ptr);
    
    // Check if it has already been freed.
    // Does not handle case when start_address passed was never allocated.
    if (((Header *)start_address)->free) {
        return;
    }

    Header *header = (Header *)start_address;
    int size = header->size;
    uintptr_t addr = (uintptr_t)header;
    if (size % PAGE_SIZE == 0 && addr % PAGE_SIZE == 0) {
        // if: full page is free (or multiple consecutive pages), page-aligned -> can munmap it.
        unmap(start_address, size);
    } else {
        append_to_free_list(start_address);
        coalesce(start_address);
        // if we are left with a free block of size bigger than PAGE_SIZE that is
        // page-aligned, munmap that part.
        if (size >= PAGE_SIZE && addr % PAGE_SIZE == 0) {
            split(start_address, size, (size / PAGE_SIZE) * PAGE_SIZE);
            unmap(start_address, (size / PAGE_SIZE) * PAGE_SIZE);
        }
    }
}

void copy_block(int *src, int *dst, int size) {
    int i;
    // Know that it is 8-bit aligned, so can copy whole ints.
    for (i = 0; i * sizeof(int) < size; i++) {
        dst[i] = src[i];
    }
}

void *myrealloc(void *ptr, int size) {
    // If ptr is NULL, realloc() is identical to a call to malloc() for size bytes.
    if (!ptr) {
        return myalloc(size);
    }
    // If size is zero and ptr is not NULL, a new, minimum sized object (MIN_SIZE) is
    // allocated and the original object is freed.
    if (size == 0 && ptr) {
        myfree(ptr);
        return myalloc(1);
    }

    int required_size = META_SIZE + size;
    // If there is enough space, expand the block.
    int current_size = getSize(ptr);

    // if user requests to shorten the block.
    if (size < current_size) {
        return ptr;
    }
    Header *current_header = (Header *)ptr;
    Footer *current_footer = (Footer *)getFooter(ptr);
    // Next block exists and is free.
    if (current_header->has_next && ((Header *)ptr + current_size)->free) {
        int available_size = current_size + getSize(ptr + current_size);
        // Size is enough.
        if (available_size >= required_size) {
            Header *next_header = (Header *)(ptr + current_size);
            remove_from_free_list(next_header);
            // Add size of next block to the size of current block.
            current_header->size += size;
            current_footer->size = current_header->size;

            // split if possible.
            split(current_header, available_size, required_size);
            return ptr;
        }
    }

    // Not enough room to enlarge -> allocate new region.
    void *new_ptr = myalloc(size);
    // Copy old data.
    copy_block(ptr, new_ptr, current_size);
    // Free old location.
    myfree(ptr);
    return new_ptr;
}

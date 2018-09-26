# mymalloc
Simple implementation of a memory allocation library similar to malloc.

## 1. Implementing malloc with mmap()

In the mymalloc.c file, I implemented the malloc function with mmap() to allocate memory dynamically by using the virtual memory.

## 2. Simple and neat malloc with sbrk()

In the simple_malloc.c file, I implemented the malloc function, which is really short and simple. I used the sbrk system call to move the break on the heap to allocate memory dynamically
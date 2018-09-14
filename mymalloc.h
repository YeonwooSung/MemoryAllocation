/*	Allocate 'size' bytes of memory. On success the function returns a pointer to 
	the start of the allocated region. On failure NULL is returned. */
extern void *myalloc(int size);

/*	Release the region of memory pointed to by 'ptr'. */
extern void myfree(void *ptr);

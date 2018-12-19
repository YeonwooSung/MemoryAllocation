# MemoryAllocation

## 1. Implementing malloc with mmap()

In the mmap_malloc.c file, I implemented the memory allocating program with mmap() function to allocate memory dynamically by using the virtual memory.

## 2. Simple and neat malloc with sbrk()

In the simple_malloc.c file, I implemented the malloc function, which is really short and simple. I used the sbrk system call to move the break on the heap to allocate memory dynamically

## Why do programmers keep try to make their own malloc?

Some people might curious why do programmers keep try to implement new malloc, rather than just using the existing memory allocating function.

The answer is because programmers want to use the memory allocating function which is highly optimised and extremely suitable to their software. For example, some programmers tried to override new() method of all classes in C++ to make the optimised memory allocating process.

Some might say that the memory space of the modern computer is huge enough, thus, just using the standard memory allocation function and method is not a bad way. To be honest, it is neither a bad way nor good way. If you are participating to the project which requires highly efficient memory management algorithm, then you might need to consider to implement an optimised memory allocating function, as well as, a highly efficient memory management microservice software.

It is clear that the programs that I wrote in this project are not highly efficient memory allocators, however, I used some straightforward algorithms for both sbrk_malloc and mmap_malloc with suitable data structures. I believe that reading the codes that I wrote in this project is a good starting point for the people who wants to implement their own memory allocation function.

## What is a good design then?

According to Doug Lea, a good memory allocator needs to balance the goals below:

    1) Maximizing Compatibility
        Should obey ANSI/POSIX conventions

    2) Minimizing Space

    3) Maximizing Portability

    4) Minimizing Execution Time

    5) Maximizing Tunability
        Optional features and behaviour should be contollable by users either statically (via #define, etc) or dynamically (via control commands such as mallopt).

    6) Maximizing Locality
        Allocating chunks of memory that are typically used together near each other. This helps minimize page and cache misses during program execution.

    7) Maximizing Error Detection
        It does not seem possible for a general-purpose memory error testing tool such as "Purify". However, allocators should provide some means for detecting corruption due to overwriting memory, multiple frees, and so on.

    8) Minimizing Anomalies
        An allocator configured using default settings should perform well across a wide range of real loads that heavily depend on dynamic allocation (i.e. windowing toolkits, GUI applications, compilers, etc).


Moreover, it is clear that minimizing space by minimizing the wastage must be the primary goal in any allocator. This means that you could maximize the space by maximizing the wastage - does not manage the freed memory by using the proper algorithm with free-list, and just allocate new page.

However, the freed memory managing algorithm should not waste much time, because the time is also an important factor in memory allocator design. Even if you make your memory allocator highly memory efficient, people would not use it if the execution time of that memory allocator is much longer than the standard memory allocating function.

Furthermore, you need to have a tradeoff between tunability (such as setting a debug mode) and time efficiency. If you try to implement the memory allocating function that has multiple levels to allow the user a debug mode, then the time efficiency of that memory allocator will be decreased. It is because that the most provisions for dynamic tunability can seriously impact the time efficiency by adding levels of indirection and increasing the number of branches.

Until now, we only considered 3 factors: space efficiency, time efficiency, and tunability. However, as I mentioned above, the memory allocator has total 8 factors that should be considered. It will be definitely tough to design a perfect memory allocator that satisfies all conditions above. However, if you only consider some of them (perhaps 2 or 3?), then it would be terribly easier than designing and implementing the perfect one.

Henceforth, before start implementing the memory allocator, try to find the most important elements to your memory allocator. Find the most important factors and concentrate on them.
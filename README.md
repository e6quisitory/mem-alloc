# Memory Allocator
A from-scratch implementation of `malloc()` and `free()` (named `alloc()` and `dealloc()` in the code).

Coded for and tested on Linux, but should work on any POSIX-compliant OS (ex. macOS).

This project was completed as part of CMPT 201: Systems Programming at Simon Fraser University.

## Function Descriptions
- `void allocopt(enum algs, int)` : Set the free-block search algorithm (`FIRST_FIT`, `BEST_FIT`, `WORST_FIT`) and the heap limit
- `void* alloc(int)` : allocate a desired number of bytes and return a pointer to them
- `void dealloc(void*)` : deallocate (free) an allocated block of memory
- `struct allocinfo allocinfo()` : Returns a struct containing a field called `free_size`, which is the total number of usable free bytes on the heap

## Technical Details
This memory allocator works by requesting heap memory in large chuncks of size `256` bytes from the OS, using the `sbrk()` system call, then managing these large chunks through a linked list of free memory blocks, embedded within the heap itself.

The allocator supports coalescing free blocks to reduce external fragmentation.

When a requested allocation is too big, i.e. no free blocks of appropriate size are available, the allocator increases the heap by `256` bytes again through `sbrk()` and then tries performing the requested allocation once more.

## Build Instructions
Files `include/alloc.h` and `src/alloc.c` make up the library containing `alloc()` and `dealloc()`. `src/main.c` contains example usage of the library. To build it, after cloning this repo, from its root, run the following commands:

```
mkdir build
cd build
cmake ..
make
./mem-alloc
```

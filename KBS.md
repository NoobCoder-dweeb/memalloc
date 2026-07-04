# Memory Layout

A process runs within its own virtual address space that is disctinct from the virtual address spaces of other processes.

__5 sections__:

* text section: part that contains the binary instructions to be executed by the processor
* data section: contain non-0 init. static data
* Block Started by Symbol (BSS): contains 0-init static data. Static data uninitialised in program is init. 0 and goes here.
* heap: contains the dynamically allocated data
* stack: contains automatic variables, function args, copy of base ptr etc.

  * stack and heap grows in opposite directions
  * bss and heap sections are collectively referred to as the __data segment__, the end of which is demarcated by a ptr named program break or __brk__
  * To allocate more memory in the heap, need to request the system to increment brk.

```bash
sbrk() # Linux/UNIX
sbrk(0) # returns current address of program break
sbrk(+x) # increments brk by x bytes, resulting in allocating memory 
sbrk(-x) # decrements brk by x bytes, resulting in deallocating memory 
# failure returns (void*) -1
```

## malloc()

`malloc(size)` allocates `size` bytes of memory and returns a ptr to the __allocated memory__.
`free(ptr)` frees the memory block pointed by __ptr__ which must have been returned by a previous call to `malloc()`, `calloc()`, or `realloc()`. But must know the __size of the memory block to be freed__.

Only release memory at the end of the heap not in the __middle of OS__.

Free-ing data != release memory back to OS. Mark it as __free__. It can be reused later on a `malloc()` call.

__Store__:

1. size
2. Whether a block is free or not free?

Request: calculate `total_size = header_size + size` -> call `sbrk(total_size)`

__transversal__:

* to free()
* use linked list
* STUB: a 16 bytes to ensure the header ends up on a memory address aligned to 16 Bytes.

## Global Lock

Prevent 2 or more threads from concurrently access memory.

`pthread_mutex_t global_malloc_lock`

## Algorithm

1. Verify valid requestied size.
2. Acquire the lock.
3. Transverse Linked list to see if there already exist a block of memory that is marked as free and accommodate the given size. __first-fit__ approach search.
4. If found, mark the block as not-free, release the global loc and return a ptr to that block. Keep it hidden e.g. `(header+1)` points to the block right after the end of the header. cast to `(void*)` and returned.
5. Else, extend heap by calling `sbrk()` to extend by a size that fits the requested __size__ + header.
6. Compute `total_size = sizeof(header_t) + size` and call `sbrk(total_size)`
7. Fill the header with the requested size and mark it as not free.
8. Update the `next` ptr, head and tail to reflect the new state of the linked list.
9. hide the header from caller and return `(void*)(header+1)`/
10. Release the global lock.

* In the memory thus obtained from the OS, there is __no need__ to cast a `void*` to any other pointer type, it is always __safely promoted__. So no need to do `header = (header_t*)block`

## free()

__Algorithm__:

1. get header block to free: `header = (header_t*) block - 1`
2. find the end of the current block: `(char*) block + header->s.size`
3. Compare with program brk: `sbrk(0)`
4. If current block is at end, reset the head and tail ptrs to reflect the loss of the last block.
5. Compute amount of memory to be released: `total_size=sizeof(header_t) + header->s.size`.
6. Call `sbrk(-total_size)`. __Negative__ means __release__.
7. Else, set the `is_free` field of the header.

## calloc()

`calloc(num, nsize)` = allocates memory for an array of `num` elements of `nsize` bytes each and returns ptr to the allocated memory. Memory is all set to 0s.

## realloc()

Changes the size of the given memory block to the size given

__Algorithm:__

1. get block's header and see if the block already has the size to accomodate the requested size
2. If yes, nothing to be done.
3. Else, call `malloc()` to get a block of the requested size, and
4. reallocate contents to the new bigger block using `memcpy()`.
5. Old memory block is freed

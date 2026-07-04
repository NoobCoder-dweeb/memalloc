#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

typedef char ALIGN[16];

union header
{
    /*
     * Size of union = max(sizof(stub), sizeof(header_t))
     * Memory aligns to 16 bytes
     */
    struct header_t
    {
        /* Stores state of a block of memory */
        size_t size;
        unsigned is_free; // free or not-free (unblock)
        // struct header_t *next; // pointer to the next block
        union header *next; // pointer to the next block
    } s;

    ALIGN stub;
};
typedef union header header_t;

// pointer to keep track of list
header_t *head, *tail;

// define global mutex
pthread_mutex_t global_malloc_lock;

header_t *get_free_block(size_t size)
{
    /* Function that transverses the linked list and see if there already exists a block of memory that is marked as free and can accommodate given size*/
    header_t *current = head;
    while (current)
    {
        // found: mark the header as not-free
        if (current->s.is_free && current->s.size >= size)
            return current;

        current = current->s.next;
    }
    return NULL;
}

void *malloc(size_t size)
{
    /* Allocates size bytes of memory and returns ptr to allocated memory */
    size_t total_size;
    void *block;
    header_t *header;
    block = sbrk(size);

    // check if size is 0
    if (!size)
    {
        return NULL;
    }

    pthread_mutex_lock(&global_malloc_lock);
    header = get_free_block(size);

    if (header)
    {
        header->s.is_free = 0;
        pthread_mutex_unlock(&global_malloc_lock);
        return (void *)(header + 1); // points to the byte after the header
    }

    total_size = sizeof(header_t) + size;
    block = sbrk(total_size);

    // check for failure accessing memory
    if (block == (void *)-1)
    {
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }

    header = block;
    header->s.size = size;
    header->s.is_free = 0;
    header->s.next = NULL;

    if (!head)
        head = header;

    if (tail)
        tail->s.next = header;

    tail = header;
    pthread_mutex_unlock(&global_malloc_lock);

    return (void *)(header + 1);
}

int main()
{
    printf("Memory Allocator 101\n");
    printf("====================\n");

    printf("%p\n", malloc(20));

    return 0;
}
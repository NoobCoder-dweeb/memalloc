#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

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
        // found sufficient block
        header->s.is_free = 0;
        pthread_mutex_unlock(&global_malloc_lock);
        return (void *)(header + 1); // points to the byte after the header
    }

    // allocate more
    total_size = sizeof(header_t) + size;
    block = sbrk(total_size);

    // check for failure accessing memory
    if (block == (void *)-1)
    {
        pthread_mutex_unlock(&global_malloc_lock);
        return NULL;
    }

    // set the block as not-free
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

void free(void *block)
{
    header_t *header, *tmp;
    void *programbreak;

    if (!block)
    {
        return;
    }

    pthread_mutex_lock(&global_malloc_lock);

    // get the pointer to the block behind the block you want to free
    header = (header_t *)block - 1;

    programbreak = sbrk(0); // gives current value of program break

    void *end = (char *)block + header->s.size;
    if (end == programbreak)
    {
        // shrink the size of the heap and release memory to OS
        // reset head and tail pointers to reflect the loss of the last block
        if (head == tail)
        {
            head = tail = NULL;
        }
        else
        {
            tmp = head;
            while (tmp)
            {
                if (tmp->s.next == tail)
                {
                    tmp->s.next = NULL;
                    tail = tmp;
                }
                tmp = tmp->s.next;
            }
        }
        size_t total_size = sizeof(header_t) + header->s.size;

        sbrk(0 - total_size); // -ve values to deallocate
        pthread_mutex_unlock(&global_malloc_lock);
        return;
    }
    // set the is_free field of header if not the last block
    header->s.is_free = 1;
    pthread_mutex_unlock(&global_malloc_lock);
}

void *calloc(size_t num, size_t nsize)
{
    size_t size;
    void *block;
    if (!num || !nsize)
    {
        return NULL;
    }
    size = num * nsize;
    /* Check multiplication overflow */
    if (nsize != size / num)
        return NULL;

    block = malloc(size);
    if (!block)
    {
        return NULL;
    }

    // set size bytes of block to 0
    memset(block, 0, size);
    return block;
}

void *realloc(void *block, size_t size)
{
    header_t *header;
    void *ret;
    if (!block || !size)
    {
        return malloc(size);
    }
    // get the block's hedaer to see if it already has the size to accommodate the requested size
    header = (header_t *)block - 1;

    // return block if can fit
    if (header->s.size >= size)
        return block;

    // else, call malloc to get a block of the requested size
    ret = malloc(size);
    if (ret)
    {
        //  reallocate contents to the new bigger block
        memcpy(ret, block, header->s.size);

        // old block is freed
        free(block);
    }

    return ret;
}

int main()
{
    printf("Memory Allocator 101\n");
    printf("====================\n");

    printf("%p\n", malloc(20));

    return 0;
}
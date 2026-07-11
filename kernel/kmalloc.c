#include <kernel/kmalloc.h>
#include <kernel/page.h>
#include <kernel/panic.h>
#include <kernel/printk.h>

#define KMALLOC_ALIGN 16UL
#define KMALLOC_MAGIC 0x4b4d414c4c4f4301UL

struct kmem_block {
    unsigned long size;
    unsigned long free;
    unsigned long magic;
    struct kmem_block *prev;
    struct kmem_block *next;
    unsigned long reserved;
};

static struct kmem_block *heap_head;
static struct kmem_block *heap_tail;

static unsigned long align_up(unsigned long value, unsigned long align)
{
    return (value + align - 1) & ~(align - 1);
}

static int block_adjacent(struct kmem_block *left, struct kmem_block *right)
{
    return (char *)left + sizeof(*left) + left->size == (char *)right;
}

static void heap_append(struct kmem_block *block)
{
    block->prev = heap_tail;
    block->next = 0;

    if (heap_tail) {
        heap_tail->next = block;
    } else {
        heap_head = block;
    }

    heap_tail = block;
}

static struct kmem_block *heap_grow(void)
{
    struct kmem_block *block = page_alloc();

    if (!block) {
        return 0;
    }

    block->size = PAGE_SIZE - sizeof(*block);
    block->free = 1;
    block->magic = KMALLOC_MAGIC;
    block->reserved = 0;
    heap_append(block);
    return block;
}

static void split_block(struct kmem_block *block, unsigned long size)
{
    struct kmem_block *new_block;
    unsigned long remaining;

    if (block->size < size + sizeof(*block) + KMALLOC_ALIGN) {
        return;
    }

    remaining = block->size - size - sizeof(*block);
    new_block = (struct kmem_block *)((char *)block + sizeof(*block) + size);
    new_block->size = remaining;
    new_block->free = 1;
    new_block->magic = KMALLOC_MAGIC;
    new_block->reserved = 0;
    new_block->prev = block;
    new_block->next = block->next;

    if (block->next) {
        block->next->prev = new_block;
    } else {
        heap_tail = new_block;
    }

    block->next = new_block;
    block->size = size;
}

static void merge_with_next(struct kmem_block *block)
{
    struct kmem_block *next = block->next;

    if (!next || !next->free || !block_adjacent(block, next)) {
        return;
    }

    block->size += sizeof(*next) + next->size;
    block->next = next->next;

    if (next->next) {
        next->next->prev = block;
    } else {
        heap_tail = block;
    }
}

void *kmalloc(unsigned long size)
{
    struct kmem_block *block;

    if (size == 0) {
        return 0;
    }

    size = align_up(size, KMALLOC_ALIGN);
    if (size > PAGE_SIZE - sizeof(struct kmem_block)) {
        return 0;
    }

    for (;;) {
        for (block = heap_head; block; block = block->next) {
            if (block->free && block->size >= size) {
                split_block(block, size);
                block->free = 0;
                return (char *)block + sizeof(*block);
            }
        }

        if (!heap_grow()) {
            return 0;
        }
    }
}

void kfree(void *ptr)
{
    struct kmem_block *block;

    if (!ptr) {
        return;
    }

    block = (struct kmem_block *)((char *)ptr - sizeof(*block));
    assert(block->magic == KMALLOC_MAGIC);
    assert(block->free == 0);

    block->free = 1;
    merge_with_next(block);

    if (block->prev && block->prev->free) {
        merge_with_next(block->prev);
    }
}

void kmalloc_test(void)
{
    void *a = kmalloc(24);
    void *b = kmalloc(128);
    void *c = kmalloc(512);
    void *d;
    void *e;

    assert(a != 0);
    assert(b != 0);
    assert(c != 0);
    assert(((unsigned long)a & (KMALLOC_ALIGN - 1)) == 0);
    assert(((unsigned long)b & (KMALLOC_ALIGN - 1)) == 0);
    assert(((unsigned long)c & (KMALLOC_ALIGN - 1)) == 0);

    kfree(b);
    d = kmalloc(64);
    assert(d == b);

    kfree(a);
    kfree(c);
    kfree(d);

    e = kmalloc(600);
    assert(e != 0);
    kfree(e);

    printk("kmalloc test passed\n");
}

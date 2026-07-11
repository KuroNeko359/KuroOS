#include <kernel/list.h>
#include <kernel/page.h>
#include <kernel/panic.h>
#include <kernel/printk.h>

/*
 * QEMU virt default RAM is 128 MiB at 0x80000000..0x88000000.
 * Temporary until boot code preserves the DTB pointer and we parse /memory.
 */
#define QEMU_VIRT_RAM_END 0x88000000UL

extern char __kernel_end[];

struct page {
    struct list_head node;
};

static LIST_HEAD(free_page_list);
static unsigned long nr_total_pages;
static unsigned long nr_free_pages;

/**
 * @brief Align upwards to an integer multiple of 2
 * 
 * @param value The number that you will align to.
 * @param align Must be a integer multiple of 2. 
 * For example, ~(1000000 - 1) = ~(0111111) = 1000000
 * @return unsigned long , the result aligned up.
 */
static unsigned long align_up(unsigned long value, unsigned long align)
{
    return (value + align - 1) & ~(align - 1);
}

/**
 * @brief Verify wether the value is aligned.
 * 
 * @param value The aligned value.
 * @return int Yes or No
 */
static int page_aligned(unsigned long value)
{
    return (value & PAGE_MASK) == 0;
}

/**
 * @brief Initial page from @param start to @param end.
 * 
 * @param start Start of range
 * @param end End of range
 */
void page_init_range(unsigned long start, unsigned long end)
{
    unsigned long p = align_up(start, PAGE_SIZE);

    assert(page_aligned(end));

    INIT_LIST_HEAD(&free_page_list);
    nr_total_pages = 0;
    nr_free_pages = 0;

    while (p + PAGE_SIZE <= end) {
        nr_total_pages++;
        page_free((void *)p);
        p += PAGE_SIZE;
    }

    printk("page allocator: total=%lu free=%lu used=%lu\n",
           page_total_count(), page_free_count(), page_used_count());
}

void page_init(void)
{
    page_init_range((unsigned long)__kernel_end, QEMU_VIRT_RAM_END);
}

void *page_alloc(void)
{
    struct page *page;

    if (list_empty(&free_page_list)) {
        return 0;
    }

    page = list_first_entry(&free_page_list, struct page, node);
    list_del(&page->node);
    nr_free_pages--;

    return page;
}

void page_free(void *addr)
{
    struct page *page = addr;

    assert(addr != 0);
    assert(page_aligned((unsigned long)addr));

    INIT_LIST_HEAD(&page->node);
    list_add(&page->node, &free_page_list);
    nr_free_pages++;
}

unsigned long page_total_count(void)
{
    return nr_total_pages;
}

unsigned long page_free_count(void)
{
    return nr_free_pages;
}

unsigned long page_used_count(void)
{
    return nr_total_pages - nr_free_pages;
}

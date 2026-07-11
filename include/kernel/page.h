#ifndef KERNEL_PAGE_H
#define KERNEL_PAGE_H

#define PAGE_SIZE 4096UL
#define PAGE_MASK (PAGE_SIZE - 1)

void page_init(void);
void page_init_range(unsigned long start, unsigned long end);
void *page_alloc(void);
void page_free(void *addr);
unsigned long page_total_count(void);
unsigned long page_free_count(void);
unsigned long page_used_count(void);

#endif

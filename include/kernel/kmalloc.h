#ifndef KERNEL_KMALLOC_H
#define KERNEL_KMALLOC_H

void *kmalloc(unsigned long size);
void kfree(void *ptr);
void kmalloc_test(void);

#endif

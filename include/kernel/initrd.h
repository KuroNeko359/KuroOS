#ifndef KERNEL_INITRD_H
#define KERNEL_INITRD_H

/* Initialize initrd from memory */
void initrd_init(unsigned long start, unsigned long size);

/* Find a file in initrd, returns pointer to data and sets size */
void *initrd_find(const char *name, unsigned long *size);

#endif

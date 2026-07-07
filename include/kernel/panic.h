#ifndef KERNEL_PANIC_H
#define KERNEL_PANIC_H

void panic(const char *msg);
void panic_at(const char *file, int line, const char *msg);

#define assert(x)                                                  \
    do {                                                           \
        if (!(x)) {                                                \
            panic_at(__FILE__, __LINE__, "assertion failed: " #x); \
        }                                                          \
    } while (0)

#endif

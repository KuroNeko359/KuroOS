#include <user/syscall.h>

/* Loop program - runs for a while to test scheduling */
void main(void)
{
    volatile unsigned long i;
    
    write(1, "Loop program started\n", 21);
    write(1, "Running 100M iterations...\n", 27);
    
    for (i = 0; i < 100000000UL; i++) {
        /* Busy loop - will be preempted by timer */
    }
    
    write(1, "Loop finished!\n", 15);
    exit(0);
}

#include <user/syscall.h>

/* Counter program - counts to 5 then exits */
void main(void)
{
    int i;
    
    write(1, "Counter started\n", 16);
    
    for (i = 1; i <= 5; i++) {
        write(1, "count: ", 7);
        char c = '0' + i;
        write(1, &c, 1);
        write(1, "\n", 1);
        
        /* Yield to show scheduling */
        yield();
    }
    
    write(1, "Counter done!\n", 14);
    exit(0);
}

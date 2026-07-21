#include <user/syscall.h>

/* Simple hello program for testing exec */
void main(void)
{
    write(1, "Hello from hello program!\n", 26);
    write(1, "PID: ", 5);
    
    /* Simple PID print */
    long pid = getpid();
    if (pid < 10) {
        char c = '0' + pid;
        write(1, &c, 1);
    }
    write(1, "\n", 1);
    
    exit(0);
}

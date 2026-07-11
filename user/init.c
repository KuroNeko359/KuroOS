#include <user/stdio.h>
#include <user/syscall.h>

static int streq(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }

    return *a == *b;
}

/* The first user program: a tiny built-in-command shell. */
void user_main(void)
{
    char line[80];

    puts("tiny shell started\n");

    for (;;) {
        puts("> ");

        long n = read(0, line, sizeof(line));
        if (n < 0) {
            puts("read error\n");
            continue;
        }

        if (streq(line, "")) {
            continue;
        }

        if (streq(line, "help")) {
        puts("commands: help hello about yield exit running\n");
        } else if (streq(line, "hello")) {
            puts("hello from user shell\n");
        } else if (streq(line, "about")) {
            puts("riscv-os shell running in U-mode\n");
        } else if (streq(line, "yield")) {
            yield();
        } else if (streq(line, "exit")) {
            puts("bye\n");
            exit(0);
        } else if (streq(line, "running")) {
            printf("running task: %ld\n", getpid());
        }else {
            puts("unknown command: ");
            puts(line);
            puts("\n");
        }
    }
}

void user_demo(void)
{
    volatile unsigned long ticks = 0;

    write(1, "demo task started\n", 18);
    while (ticks < 20000000UL) {
        ticks++;
    }
    write(1, "demo task exiting\n", 18);
    exit(42);

    for (;;) {
    }
}

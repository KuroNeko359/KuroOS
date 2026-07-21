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

static void cmd_help(void)
{
    puts("available commands:\n");
    puts("  help      - show this help\n");
    puts("  hello     - print hello message\n");
    puts("  about     - about this OS\n");
    puts("  pid       - show current process ID\n");
    puts("  ps        - list all processes\n");
    puts("  yield     - yield CPU to other tasks\n");
    puts("  fork      - fork a child process\n");
    puts("  forktest  - fork and show processes in parent\n");
    puts("  forkloop  - fork multiple children\n");
    puts("  exec NAME - execute program from initrd\n");
    puts("  exectest  - fork and exec hello program\n");
    puts("  readtest  - test reading input\n");
    puts("  looptest  - busy loop test (preemptive scheduling)\n");
    puts("  exit      - exit the shell\n");
}

static void cmd_hello(void)
{
    puts("Hello from KuroOS user shell!\n");
}

static void cmd_about(void)
{
    puts("KuroOS - Minimal RISC-V Operating System\n");
    puts("Features:\n");
    puts("  - Sv39 virtual memory\n");
    puts("  - Preemptive scheduling (timer interrupt)\n");
    puts("  - User mode (U-mode) processes\n");
    puts("  - fork() and exec() system calls\n");
}

static void cmd_pid(void)
{
    printf("current pid: %ld\n", getpid());
}

static void cmd_ps(void)
{
    puts("process list:\n");
    ps();
}

static void cmd_yield(void)
{
    puts("yielding CPU...\n");
    yield();
    puts("resumed after yield\n");
}

static void cmd_fork(void)
{
    long pid = fork();

    if (pid < 0) {
        puts("fork failed!\n");
    } else if (pid == 0) {
        printf("[child]  pid=%ld, hello from child!\n", getpid());
        exit(0);
    } else {
        printf("[parent] forked child pid=%ld\n", pid);
    }
}

static void cmd_forktest(void)
{
    long pid1, pid2;

    puts("=== fork test ===\n");
    puts("before fork:\n");
    ps();

    puts("\nforking 2 children...\n");
    pid1 = fork();
    if (pid1 == 0) {
        printf("[child1] pid=%ld alive!\n", getpid());
        for (int i = 0; i < 20; i++) yield();
        exit(0);
    }

    pid2 = fork();
    if (pid2 == 0) {
        printf("[child2] pid=%ld alive!\n", getpid());
        for (int i = 0; i < 20; i++) yield();
        exit(0);
    }

    printf("[parent] created children: %ld, %ld\n", pid1, pid2);
    puts("\nparent calling ps (children should be visible):\n");
    ps();

    puts("\nparent waiting for children...\n");
    for (int i = 0; i < 10; i++) yield();

    puts("\nparent calling ps again:\n");
    ps();

    puts("\nparent done\n");
}

static void cmd_forkloop(void)
{
    int i;
    long pid;

    puts("forking 3 children...\n");

    for (i = 0; i < 3; i++) {
        pid = fork();
        if (pid < 0) {
            puts("fork failed!\n");
            break;
        } else if (pid == 0) {
            printf("[child %d] pid=%ld alive!\n", i, getpid());
            for (int j = 0; j < 15; j++) {
                yield();
            }
            exit(i);
        } else {
            printf("[parent] created child pid=%ld\n", pid);
        }
    }

    puts("parent done forking\n");
    puts("run 'ps' to see all processes!\n");
    
    for (int i = 0; i < 20; i++) {
        yield();
    }
    puts("parent done\n");
}

static void cmd_exec(const char *name)
{
    printf("exec: running '%s'...\n", name);
    long ret = exec(name);
    if (ret < 0) {
        printf("exec: failed to run '%s'\n", name);
    }
    /* exec does not return on success */
}

static void cmd_exectest(void)
{
    long pid = fork();

    if (pid < 0) {
        puts("fork failed!\n");
        return;
    } else if (pid == 0) {
        /* Child: exec hello program */
        puts("[child] exec hello...\n");
        exec("hello");
        /* Should not reach here */
        puts("[child] exec failed!\n");
        exit(1);
    } else {
        /* Parent: wait a bit */
        printf("[parent] forked child pid=%ld, waiting...\n", pid);
        for (int i = 0; i < 10; i++) yield();
        puts("[parent] done\n");
    }
}

static void cmd_readtest(void)
{
    char buf[64];
    long n;

    puts("enter something: ");
    n = read(0, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        printf("you entered: %s\n", buf);
    } else {
        puts("read error\n");
    }
}

static void cmd_looptest(void)
{
    volatile unsigned long i;
    int pid = getpid();

    printf("[pid %d] starting busy loop (will be preempted by timer)...\n", pid);

    for (i = 0; i < 500000000UL; i++) {
        /* Busy wait - timer interrupt will preempt us */
    }

    printf("[pid %d] loop finished\n", pid);
}

/* The first user program: a tiny built-in-command shell. */
void user_main(void)
{
    char line[80];

    puts("=== KuroOS Shell ===\n");
    puts("type 'help' for commands\n\n");

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
            cmd_help();
        } else if (streq(line, "hello")) {
            cmd_hello();
        } else if (streq(line, "about")) {
            cmd_about();
        } else if (streq(line, "pid") || streq(line, "getpid")) {
            cmd_pid();
        } else if (streq(line, "ps")) {
            cmd_ps();
        } else if (streq(line, "yield")) {
            cmd_yield();
        } else if (streq(line, "fork")) {
            cmd_fork();
        } else if (streq(line, "forktest")) {
            cmd_forktest();
        } else if (streq(line, "forkloop")) {
            cmd_forkloop();
        } else if (streq(line, "readtest")) {
            cmd_readtest();
        } else if (streq(line, "looptest")) {
            cmd_looptest();
        } else if (streq(line, "exectest")) {
            cmd_exectest();
        } else if (streq(line, "exit") || streq(line, "quit")) {
            puts("goodbye!\n");
            exit(0);
        } else {
            /* Check if it starts with "exec " */
            if (line[0] == 'e' && line[1] == 'x' && line[2] == 'e' && 
                line[3] == 'c' && line[4] == ' ') {
                cmd_exec(line + 5);
            } else {
                puts("unknown command: ");
                puts(line);
                puts("\n");
            }
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

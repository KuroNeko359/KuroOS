#include <kernel/printk.h>
#include <kernel/user.h>

extern void enter_user(void *user_stack_top, void *user_entry);
extern void user_main(void);

/* First user task stack. A real task table will give each process its own. */
static unsigned char user_stack[4096] __attribute__((aligned(16)));

/* Start the initial user program by switching from S-mode to U-mode. */
void user_init(void)
{
    void *user_stack_top = user_stack + sizeof(user_stack);

    printk("entering user mode\n");
    enter_user(user_stack_top, user_main);

    for (;;) {
    }
}

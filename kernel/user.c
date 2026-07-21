#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/task.h>
#include <kernel/user.h>

extern void enter_user(void *user_stack_top, void *user_entry, void *kernel_stack_top);
extern void user_main(void);
extern void user_demo(void);
extern char __user_start[];
extern char __user_end[];

/* Start the initial user program by switching from S-mode to U-mode. */
void user_init(void)
{
    struct task *init = task_create_user((unsigned long)__user_start,
                                         (unsigned long)__user_end,
                                         user_main);
    struct task *demo = task_create_user((unsigned long)__user_start,
                                         (unsigned long)__user_end,
                                         user_demo);

    assert(init != 0);
    assert(demo != 0);
    task_set_current(init);
    task_dump_run_queue();

    printk("entering user mode: pid=%lu\n", init->pid);
    enter_user((void *)init->user_sp, (void *)init->entry, (void *)init->kernel_sp);

    for (;;) {
    }
}

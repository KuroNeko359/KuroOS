#include <kernel/page.h>
#include <kernel/kmalloc.h>
#include <kernel/panic.h>
#include <kernel/pgtable.h>
#include <kernel/printk.h>
#include <kernel/task.h>
#include <kernel/trap.h>

#define SSTATUS_SPIE (1UL << 5)

/*
Lifetime:
TASK
  -> task_create()
  -> TASK_READY
  -> TASK_RUNNING
  -> exit()
  -> TASK_ZOMBIE
  -> task_reap_zombies()
  -> resources and task structure freed
*/

static struct task *current;
static unsigned long next_pid = 1;
static unsigned long nr_runnable_tasks;
static LIST_HEAD(run_queue);
static LIST_HEAD(all_tasks);

static void task_init_frame(struct task *task)
{
    struct trap_frame *tf;
    unsigned long *slot;

    task->tf = (struct trap_frame *)(task->kernel_sp - sizeof(struct trap_frame));
    tf = task->tf;
    slot = (unsigned long *)tf;

    for (unsigned long i = 0; i < sizeof(*tf) / sizeof(slot[0]); i++) {
        slot[i] = 0;
    }

    tf->sp = task->user_sp;
    tf->sepc = task->entry;
    tf->sstatus = SSTATUS_SPIE;
}

static struct task *task_alloc(void)
{
    struct task *task;

    task = kmalloc(sizeof(struct task));
    if (!task) {
        return 0;
    }

    task->pid = 0;
    task->state = TASK_UNUSED;
    task->tf = 0;
    task->kernel_sp = 0;
    task->user_sp = 0;
    task->entry = 0;
    task->kernel_stack_base = 0;
    task->user_stack_base = 0;
    task->stack_size = 0;
    task->exit_code = 0;
    task->pagetable = 0;

    INIT_LIST_HEAD(&task->task_node);
    INIT_LIST_HEAD(&task->run_node);

    return task;
}

void task_reap_zombies(void)
{
    struct list_head *pos;
    struct list_head *next;

    list_for_each_safe(pos, next, &all_tasks) {
        struct task *task = list_entry(pos, struct task, task_node);

        if (task->state != TASK_ZOMBIE || task == current) {
            continue;
        }

        printk("task reap: pid=%lu exit_code=%d\n",
               task->pid, (int)task->exit_code);

        if (task->user_stack_base) {
            page_free((void *)task->user_stack_base);
        }
        if (task->kernel_stack_base) {
            page_free((void *)task->kernel_stack_base);
        }
        if (task->pagetable) {
            pgtable_free_walk(task->pagetable);
        }

        list_del(&task->task_node);
        kfree(task);
    }
}

struct task *task_create(void (*entry)(void))
{
    struct task *task;
    void *user_stack;
    void *kernel_stack;
    pagetable_t pagetable;

    task_reap_zombies();
    task = task_alloc();

    if (!task) {
        return 0;
    }

    user_stack = page_alloc();
    kernel_stack = page_alloc();
    pagetable = pgtable_create();

    if (!user_stack || !kernel_stack || !pagetable) {
        if (user_stack) {
            page_free(user_stack);
        }
        if (kernel_stack) {
            page_free(kernel_stack);
        }
        if (pagetable) {
            pgtable_free_walk(pagetable);
        }
        kfree(task);
        return 0;
    }

    task->pid = next_pid++;
    task->state = TASK_READY;
    task->kernel_stack_base = (unsigned long)kernel_stack;
    task->user_stack_base = (unsigned long)user_stack;
    task->stack_size = PAGE_SIZE;
    task->kernel_sp = (unsigned long)kernel_stack + PAGE_SIZE;
    task->user_sp = (unsigned long)user_stack + PAGE_SIZE;
    task->entry = (unsigned long)entry;
    task->exit_code = 0;
    task->pagetable = pagetable;
    task_init_frame(task);

    list_add_tail(&task->task_node, &all_tasks);
    list_add_tail(&task->run_node, &run_queue);
    nr_runnable_tasks++;

    printk("task create: pid=%lu entry=%p user_sp=%p kernel_sp=%p\n",
           task->pid, entry, (void *)task->user_sp, (void *)task->kernel_sp);

    return task;
}

struct task *task_current(void)
{
    return current;
}

void task_set_current(struct task *task)
{
    assert(task != 0);

    current = task;
    current->state = TASK_RUNNING;
}

struct trap_frame *task_exit_current(struct trap_frame *tf, long code)
{
    struct task *dead;
    struct task *next;

    assert(current != 0);

    dead = current;
    dead->tf = tf;
    dead->exit_code = code;
    dead->state = TASK_ZOMBIE;

    list_del(&dead->run_node);
    assert(nr_runnable_tasks > 0);
    nr_runnable_tasks--;

    current = 0;
    if (nr_runnable_tasks == 0) {
        return 0;
    }

    next = task_pick_next();
    assert(next != 0);
    task_set_current(next);
    return next->tf;
}

struct trap_frame *task_schedule(struct trap_frame *tf)
{
    struct task *prev = current;
    struct task *next;

    if (prev) {
        prev->tf = tf;
        if (prev->state == TASK_RUNNING) {
            prev->state = TASK_READY;
        }
    }

    task_reap_zombies();

    next = task_pick_next();
    if (!next || next == prev || !next->tf) {
        if (prev) {
            prev->state = TASK_RUNNING;
        }
        return tf;
    }

    task_set_current(next);
    return next->tf;
}

struct task *task_pick_next(void)
{
    struct list_head *pos;
    unsigned long scanned = 0;

    if (list_empty(&run_queue) || nr_runnable_tasks == 0) {
        return 0;
    }

    pos = current ? current->run_node.next : run_queue.next;

    while (scanned < nr_runnable_tasks) {
        struct task *task;

        if (pos == &run_queue) {
            pos = run_queue.next;
        }

        task = list_entry(pos, struct task, run_node);
        if (task->state == TASK_READY || task->state == TASK_RUNNING) {
            return task;
        }

        pos = pos->next;
        scanned++;
    }

    return 0;
}

void task_dump_run_queue(void)
{
    struct list_head *pos;

    printk("run queue:");
    if (list_empty(&run_queue)) {
        printk(" empty\n");
        return;
    }

    list_for_each(pos, &run_queue) {
        struct task *task = list_entry(pos, struct task, run_node);
        printk(" pid=%lu(state=%d)", task->pid, task->state);
    }
    printk("\n");
}

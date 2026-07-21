#include <arch/riscv/csr.h>
#include <kernel/page.h>
#include <kernel/kmalloc.h>
#include <kernel/panic.h>
#include <kernel/pgtable.h>
#include <kernel/printk.h>
#include <kernel/task.h>
#include <kernel/trap.h>
#include <kernel/vm.h>

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

#define USER_TEXT_BASE  0x00010000UL
#define USER_STACK_TOP  0x40000000UL

static void mem_zero(void *dst, unsigned long size)
{
    unsigned char *p = dst;

    for (unsigned long i = 0; i < size; i++) {
        p[i] = 0;
    }
}

static void mem_copy(void *dst, const void *src, unsigned long size)
{
    unsigned char *d = dst;
    const unsigned char *s = src;

    for (unsigned long i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

static unsigned long align_up(unsigned long value, unsigned long align)
{
    return (value + align - 1) & ~(align - 1);
}

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
    /* SUM lets S-mode syscall handlers copy data from/to user pages. */
    tf->sstatus = SSTATUS_SPIE | SSTATUS_SUM;
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
    task->user_image_size = 0;
    task->exit_code = 0;
    task->pagetable = 0;

    INIT_LIST_HEAD(&task->task_node);
    INIT_LIST_HEAD(&task->run_node);

    return task;
}

static void task_free_user_pages(pagetable_t pagetable,
                                 unsigned long va,
                                 unsigned long size)
{
    unsigned long end = va + size;

    for (unsigned long addr = va; addr < end; addr += PAGE_SIZE) {
        pte_t *pte = pgtable_walk(pagetable, addr, 0);

        if (!pte || !pte_present(*pte) || !pte_leaf(*pte)) {
            continue;
        }

        page_free((void *)pte_to_pa(*pte));
        *pte = __pte(0);
    }
}

static void task_free(struct task *task)
{
    if (task->user_stack_base) {
        page_free((void *)task->user_stack_base);
    }
    if (task->pagetable && task->user_image_size) {
        task_free_user_pages(task->pagetable,
                             USER_TEXT_BASE,
                             task->user_image_size);
    }
    if (task->kernel_stack_base) {
        page_free((void *)task->kernel_stack_base);
    }
    if (task->pagetable) {
        pgtable_free_walk(task->pagetable);
    }

    kfree(task);
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

        list_del(&task->task_node);
        task_free(task);
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
    task->user_image_size = 0;
    task->kernel_sp = (unsigned long)kernel_stack + PAGE_SIZE;
    task->user_sp = (unsigned long)user_stack + PAGE_SIZE;
    task->entry = (unsigned long)entry;
    task->exit_code = 0;
    task->pagetable = pagetable;
    vm_allow_user_identity(task->user_stack_base, task->stack_size);
    task_init_frame(task);

    list_add_tail(&task->task_node, &all_tasks);
    list_add_tail(&task->run_node, &run_queue);
    nr_runnable_tasks++;

    return task;
}

struct task *task_create_user(unsigned long image_start,
                              unsigned long image_end,
                              void (*entry)(void))
{
    struct task *task;
    void *user_stack;
    void *kernel_stack;
    pagetable_t pagetable;
    unsigned long image_size;
    unsigned long image_pages;
    unsigned long entry_offset;
    unsigned long user_stack_va;

    task_reap_zombies();
    task = task_alloc();

    if (!task) {
        return 0;
    }

    image_size = image_end - image_start;
    image_pages = align_up(image_size, PAGE_SIZE);
    entry_offset = (unsigned long)entry - image_start;
    user_stack_va = USER_STACK_TOP - PAGE_SIZE;

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

    if (vm_map_kernel(pagetable) != 0) {
        page_free(user_stack);
        page_free(kernel_stack);
        pgtable_free_walk(pagetable);
        kfree(task);
        return 0;
    }

    for (unsigned long off = 0; off < image_pages; off += PAGE_SIZE) {
        void *page = page_alloc();
        unsigned long copy_size = PAGE_SIZE;

        if (!page) {
            task_free_user_pages(pagetable, USER_TEXT_BASE, off);
            page_free(user_stack);
            page_free(kernel_stack);
            pgtable_free_walk(pagetable);
            kfree(task);
            return 0;
        }

        mem_zero(page, PAGE_SIZE);
        if (off + copy_size > image_size) {
            copy_size = image_size - off;
        }
        mem_copy(page, (const void *)(image_start + off), copy_size);

        if (pgtable_map(pagetable,
                        USER_TEXT_BASE + off,
                        (unsigned long)page,
                        PAGE_SIZE,
                        PAGE_USER) != 0) {
            page_free(page);
            task_free_user_pages(pagetable, USER_TEXT_BASE, off);
            page_free(user_stack);
            page_free(kernel_stack);
            pgtable_free_walk(pagetable);
            kfree(task);
            return 0;
        }
    }

    mem_zero(user_stack, PAGE_SIZE);
    if (pgtable_map(pagetable,
                    user_stack_va,
                    (unsigned long)user_stack,
                    PAGE_SIZE,
                    PAGE_USER) != 0) {
        task_free_user_pages(pagetable, USER_TEXT_BASE, image_pages);
        page_free(user_stack);
        page_free(kernel_stack);
        pgtable_free_walk(pagetable);
        kfree(task);
        return 0;
    }

    task->pid = next_pid++;
    task->state = TASK_READY;
    task->kernel_stack_base = (unsigned long)kernel_stack;
    task->user_stack_base = (unsigned long)user_stack;
    task->stack_size = PAGE_SIZE;
    task->user_image_size = image_pages;
    task->kernel_sp = (unsigned long)kernel_stack + PAGE_SIZE;
    task->user_sp = USER_STACK_TOP;
    task->entry = USER_TEXT_BASE + entry_offset;
    task->exit_code = 0;
    task->pagetable = pagetable;
    task_init_frame(task);

    list_add_tail(&task->task_node, &all_tasks);
    list_add_tail(&task->run_node, &run_queue);
    nr_runnable_tasks++;

    printk("task create user: pid=%lu entry=%p user_sp=%p kernel_sp=%p\n",
           task->pid, (void *)task->entry,
           (void *)task->user_sp, (void *)task->kernel_sp);

    return task;

}

void task_destroy(struct task *task)
{
    assert(task != 0);
    assert(task != current);

    list_del(&task->task_node);
    list_del(&task->run_node);
    assert(nr_runnable_tasks > 0);
    nr_runnable_tasks--;

    task_free(task);
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
    vm_switch(current->pagetable);
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

/*
 * Fork the current task.
 *
 * Creates a new task that is a copy of the current task:
 * - Same user memory contents (copied page by page)
 * - Same register state (trap frame)
 * - New PID
 *
 * Returns:
 * - In parent: child's PID
 * - In child:  0
 */
struct task *task_fork(struct trap_frame *tf)
{
    struct task *parent = current;
    struct task *child;
    void *child_user_stack;
    void *child_kernel_stack;
    pagetable_t child_pagetable;
    unsigned long image_pages;

    if (!parent) {
        return 0;
    }

    task_reap_zombies();
    child = task_alloc();
    if (!child) {
        return 0;
    }

    child_user_stack = page_alloc();
    child_kernel_stack = page_alloc();
    child_pagetable = pgtable_create();

    if (!child_user_stack || !child_kernel_stack || !child_pagetable) {
        if (child_user_stack) page_free(child_user_stack);
        if (child_kernel_stack) page_free(child_kernel_stack);
        if (child_pagetable) pgtable_free_walk(child_pagetable);
        kfree(child);
        return 0;
    }

    /* Map kernel pages into child's page table */
    if (vm_map_kernel(child_pagetable) != 0) {
        page_free(child_user_stack);
        page_free(child_kernel_stack);
        pgtable_free_walk(child_pagetable);
        kfree(child);
        return 0;
    }

    /* Copy user text/data pages */
    image_pages = parent->user_image_size;
    for (unsigned long off = 0; off < image_pages; off += PAGE_SIZE) {
        void *src_page;
        void *dst_page;
        pte_t *src_pte;

        src_pte = pgtable_walk(parent->pagetable, USER_TEXT_BASE + off, 0);
        if (!src_pte || !pte_present(*src_pte) || !pte_leaf(*src_pte)) {
            /* Source page not present, skip */
            continue;
        }

        src_page = (void *)pte_to_pa(*src_pte);
        dst_page = page_alloc();
        if (!dst_page) {
            task_free_user_pages(child_pagetable, USER_TEXT_BASE, off);
            page_free(child_user_stack);
            page_free(child_kernel_stack);
            pgtable_free_walk(child_pagetable);
            kfree(child);
            return 0;
        }

        mem_copy(dst_page, src_page, PAGE_SIZE);

        if (pgtable_map(child_pagetable,
                        USER_TEXT_BASE + off,
                        (unsigned long)dst_page,
                        PAGE_SIZE,
                        PAGE_USER) != 0) {
            page_free(dst_page);
            task_free_user_pages(child_pagetable, USER_TEXT_BASE, off);
            page_free(child_user_stack);
            page_free(child_kernel_stack);
            pgtable_free_walk(child_pagetable);
            kfree(child);
            return 0;
        }
    }

    /* Copy user stack page */
    {
        void *src_stack;
        pte_t *stack_pte;

        stack_pte = pgtable_walk(parent->pagetable,
                                 parent->user_sp & ~(PAGE_SIZE - 1),
                                 0);
        if (stack_pte && pte_present(*stack_pte) && pte_leaf(*stack_pte)) {
            src_stack = (void *)pte_to_pa(*stack_pte);
            mem_copy(child_user_stack, src_stack, PAGE_SIZE);
        } else {
            mem_zero(child_user_stack, PAGE_SIZE);
        }
    }

    if (pgtable_map(child_pagetable,
                    USER_STACK_TOP - PAGE_SIZE,
                    (unsigned long)child_user_stack,
                    PAGE_SIZE,
                    PAGE_USER) != 0) {
        task_free_user_pages(child_pagetable, USER_TEXT_BASE, image_pages);
        page_free(child_user_stack);
        page_free(child_kernel_stack);
        pgtable_free_walk(child_pagetable);
        kfree(child);
        return 0;
    }

    /* Set up child task */
    child->pid = next_pid++;
    child->state = TASK_READY;
    child->kernel_stack_base = (unsigned long)child_kernel_stack;
    child->user_stack_base = (unsigned long)child_user_stack;
    child->stack_size = PAGE_SIZE;
    child->user_image_size = image_pages;
    child->kernel_sp = (unsigned long)child_kernel_stack + PAGE_SIZE;
    child->user_sp = parent->user_sp;
    child->entry = parent->entry;
    child->exit_code = 0;
    child->pagetable = child_pagetable;

    /* Copy trap frame to child's kernel stack */
    child->tf = (struct trap_frame *)(child->kernel_sp - sizeof(struct trap_frame));
    mem_copy(child->tf, tf, sizeof(struct trap_frame));

    /* Child returns 0 from fork */
    child->tf->a0 = 0;

    /* Parent returns child PID */
    tf->a0 = child->pid;

    list_add_tail(&child->task_node, &all_tasks);
    list_add_tail(&child->run_node, &run_queue);
    nr_runnable_tasks++;

    printk("fork: parent=%lu child=%lu\n", parent->pid, child->pid);

    return child;
}

/*
 * Print all tasks (for ps command).
 */
void task_list_all(void)
{
    struct list_head *pos;
    const char *state_str;

    printk("PID    STATE        ENTRY      USER_SP\n");
    printk("------ ------------ ---------- ----------\n");

    list_for_each(pos, &all_tasks) {
        struct task *task = list_entry(pos, struct task, task_node);

        switch (task->state) {
        case TASK_UNUSED:   state_str = "UNUSED"; break;
        case TASK_READY:    state_str = "READY"; break;
        case TASK_RUNNING:  state_str = "RUNNING"; break;
        case TASK_SLEEPING: state_str = "SLEEPING"; break;
        case TASK_ZOMBIE:   state_str = "ZOMBIE"; break;
        default:            state_str = "UNKNOWN"; break;
        }

        printk("%lu     %s", task->pid, state_str);
        /* Pad state to 12 chars */
        for (int i = 0; state_str[i]; i++) {}
        
        printk(" 0x%lx", task->entry);
        printk(" 0x%lx\n", task->user_sp);
    }

    printk("total: %lu runnable\n", nr_runnable_tasks);
}

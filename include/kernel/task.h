#ifndef KERNEL_TASK_H
#define KERNEL_TASK_H

#include <kernel/pgtable.h>
#include <kernel/list.h>
struct trap_frame;

enum task_state {
    TASK_UNUSED = 0,
    TASK_READY,
    TASK_RUNNING,
    TASK_SLEEPING,
    TASK_ZOMBIE,
};

/* Minimal process control block. */
struct task {
    unsigned long pid;
    enum task_state state;
    struct trap_frame *tf;
    unsigned long kernel_sp;
    unsigned long user_sp;
    unsigned long entry;
    unsigned long kernel_stack_base;
    unsigned long user_stack_base;
    unsigned long stack_size;
    long exit_code;
    pagetable_t pagetable;
    struct list_head task_node;
    struct list_head run_node;
};

struct task *task_create(void (*entry)(void));
struct task *task_current(void);
void task_set_current(struct task *task);
struct trap_frame *task_exit_current(struct trap_frame *tf, long code);
void task_reap_zombies(void);
struct trap_frame *task_schedule(struct trap_frame *tf);
struct task *task_pick_next(void);
void task_dump_run_queue(void);

#endif

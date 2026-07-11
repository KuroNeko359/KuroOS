#ifndef KERNEL_LIST_H
#define KERNEL_LIST_H

/*
 * Intrusive doubly linked list.
 *
 * A list_head is embedded inside the owning object. The list code only links
 * list_head nodes; list_entry/container_of recover the object that contains a
 * node.
 */
struct list_head {
    struct list_head *prev;
    struct list_head *next;
};

#define offsetof(type, member) ((unsigned long)&(((type *)0)->member))

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

static inline void __list_add(struct list_head *node,
                              struct list_head *prev,
                              struct list_head *next)
{
    next->prev = node;
    node->next = next;
    node->prev = prev;
    prev->next = node;
}

static inline void list_add(struct list_head *node, struct list_head *head)
{
    __list_add(node, head, head->next);
}

static inline void list_add_tail(struct list_head *node, struct list_head *head)
{
    __list_add(node, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = entry;
    entry->prev = entry;
}

static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}

#define list_first_entry(head, type, member) \
    list_entry((head)->next, type, member)

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, next, head) \
    for (pos = (head)->next, next = pos->next; pos != (head); \
         pos = next, next = pos->next)

#define list_for_each_entry(pos, head, member) \
    for (pos = list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, next, head, member) \
    for (pos = list_entry((head)->next, typeof(*pos), member), \
         next = list_entry(pos->member.next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = next, next = list_entry(next->member.next, typeof(*next), member))

#endif

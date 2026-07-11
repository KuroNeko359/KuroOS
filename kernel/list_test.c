#include <kernel/list.h>
#include <kernel/list_test.h>
#include <kernel/panic.h>
#include <kernel/printk.h>

struct list_test_node {
    int value;
    struct list_head link;
};

void list_test(void)
{
    LIST_HEAD(test_list);
    struct list_test_node a = { .value = 1 };
    struct list_test_node b = { .value = 2 };
    struct list_test_node c = { .value = 3 };
    struct list_test_node *node;
    int sum = 0;

    INIT_LIST_HEAD(&a.link);
    INIT_LIST_HEAD(&b.link);
    INIT_LIST_HEAD(&c.link);

    assert(list_empty(&test_list));

    list_add_tail(&a.link, &test_list);
    list_add_tail(&b.link, &test_list);
    list_add_tail(&c.link, &test_list);

    assert(!list_empty(&test_list));
    assert(list_first_entry(&test_list, struct list_test_node, link) == &a);

    list_for_each_entry(node, &test_list, link) {
        sum += node->value;
    }
    assert(sum == 6);

    list_del(&b.link);

    sum = 0;
    list_for_each_entry(node, &test_list, link) {
        sum += node->value;
    }
    assert(sum == 4);

    printk("list test passed\n");
}

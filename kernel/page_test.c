#include <kernel/page.h>
#include <kernel/page_test.h>
#include <kernel/panic.h>
#include <kernel/printk.h>

void page_test(void)
{
    unsigned long before = page_free_count();
    void *a = page_alloc();
    void *b = page_alloc();

    assert(before >= 2);
    assert(a != 0);
    assert(b != 0);
    assert(a != b);
    assert(page_free_count() == before - 2);
    assert(page_used_count() == 2);

    page_free(a);
    page_free(b);

    assert(page_free_count() == before);
    assert(page_used_count() == 0);

    printk("page test passed\n");
}

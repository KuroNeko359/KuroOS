#include <kernel/exec.h>
#include <kernel/initrd.h>
#include <kernel/page.h>
#include <kernel/pgtable.h>
#include <kernel/printk.h>
#include <kernel/panic.h>
#include <kernel/task.h>
#include <kernel/trap.h>
#include <kernel/vm.h>

/* ELF header */
typedef struct {
    unsigned char e_ident[16];  /* Magic number and other info */
    unsigned short e_type;      /* Object file type */
    unsigned short e_machine;   /* Architecture */
    unsigned int e_version;     /* Object file version */
    unsigned long e_entry;      /* Entry point virtual address */
    unsigned long e_phoff;      /* Program header table file offset */
    unsigned long e_shoff;      /* Section header table file offset */
    unsigned int e_flags;       /* Processor-specific flags */
    unsigned short e_ehsize;    /* ELF header size in bytes */
    unsigned short e_phentsize; /* Program header table entry size */
    unsigned short e_phnum;     /* Program header table entry count */
    unsigned short e_shentsize; /* Section header table entry size */
    unsigned short e_shnum;     /* Section header table entry count */
    unsigned short e_shstrndx;  /* Section header string table index */
} Elf64_Ehdr;

/* ELF program header */
typedef struct {
    unsigned int p_type;    /* Segment type */
    unsigned int p_flags;   /* Segment flags */
    unsigned long p_offset; /* Segment file offset */
    unsigned long p_vaddr;  /* Segment virtual address */
    unsigned long p_paddr;  /* Segment physical address */
    unsigned long p_filesz; /* Segment size in file */
    unsigned long p_memsz;  /* Segment size in memory */
    unsigned long p_align;  /* Segment alignment */
} Elf64_Phdr;

/* ELF magic */
#define ELFMAG0     0x7f
#define ELFMAG1     'E'
#define ELFMAG2     'L'
#define ELFMAG3     'F'

/* ELF types */
#define ET_EXEC     2       /* Executable file */
#define EM_RISCV    243     /* RISC-V */

/* Program header types */
#define PT_LOAD     1       /* Loadable segment */

/* Program header flags */
#define PF_X        0x1     /* Execute */
#define PF_W        0x2     /* Write */
#define PF_R        0x4     /* Read */

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

static unsigned long align_down(unsigned long value, unsigned long align)
{
    return value & ~(align - 1);
}

/* Verify ELF header */
static int elf_verify(const Elf64_Ehdr *ehdr, unsigned long size)
{
    if (size < sizeof(Elf64_Ehdr)) {
        return -1;
    }

    if (ehdr->e_ident[0] != ELFMAG0 ||
        ehdr->e_ident[1] != ELFMAG1 ||
        ehdr->e_ident[2] != ELFMAG2 ||
        ehdr->e_ident[3] != ELFMAG3) {
        return -1;
    }

    if (ehdr->e_type != ET_EXEC) {
        return -1;
    }

    if (ehdr->e_machine != EM_RISCV) {
        return -1;
    }

    return 0;
}

/*
 * Load ELF segments into a new page table.
 * Returns the entry point address.
 */
static unsigned long elf_load(pagetable_t pagetable,
                              const void *elf_data,
                              unsigned long elf_size)
{
    const Elf64_Ehdr *ehdr = (const Elf64_Ehdr *)elf_data;
    const Elf64_Phdr *phdr;
    unsigned long entry;
    int i;

    entry = ehdr->e_entry;

    for (i = 0; i < ehdr->e_phnum; i++) {
        phdr = (const Elf64_Phdr *)(elf_data + ehdr->e_phoff + 
                                     i * ehdr->e_phentsize);

        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        unsigned long va_start = align_down(phdr->p_vaddr, PAGE_SIZE);
        unsigned long va_end = align_up(phdr->p_vaddr + phdr->p_memsz, PAGE_SIZE);
        unsigned long nr_pages = (va_end - va_start) / PAGE_SIZE;
        unsigned long file_off = phdr->p_offset;
        unsigned long file_end = phdr->p_offset + phdr->p_filesz;
        unsigned long mem_off = phdr->p_vaddr - va_start;

        for (unsigned long j = 0; j < nr_pages; j++) {
            void *page = page_alloc();
            if (!page) {
                printk("exec: out of memory\n");
                return 0;
            }

            mem_zero(page, PAGE_SIZE);

            /* Copy file data */
            unsigned long page_va = va_start + j * PAGE_SIZE;
            unsigned long copy_start = 0;
            unsigned long copy_end = 0;

            if (page_va + PAGE_SIZE > phdr->p_vaddr && 
                page_va < phdr->p_vaddr + phdr->p_filesz) {
                unsigned long seg_off = (page_va > phdr->p_vaddr) ? 
                                        (page_va - phdr->p_vaddr) : 0;
                unsigned long seg_end = (page_va + PAGE_SIZE < phdr->p_vaddr + phdr->p_filesz) ?
                                        PAGE_SIZE : (phdr->p_vaddr + phdr->p_filesz - page_va);
                
                copy_start = (j == 0) ? mem_off : 0;
                copy_end = seg_end;
                
                if (copy_end > copy_start) {
                    mem_copy(page + copy_start, 
                            elf_data + file_off + seg_off,
                            copy_end - copy_start);
                }
            }

            /* Map page */
            int flags = PAGE_USER;
            if (phdr->p_flags & PF_W) flags |= PAGE_WRITE;
            if (phdr->p_flags & PF_X) flags |= PAGE_EXEC;

            if (pgtable_map(pagetable, va_start + j * PAGE_SIZE,
                           (unsigned long)page, PAGE_SIZE, flags) != 0) {
                page_free(page);
                printk("exec: map failed\n");
                return 0;
            }
        }
    }

    return entry;
}

/*
 * exec() system call implementation.
 *
 * Replaces the current process's address space with a new program loaded
 * from initrd.
 *
 * Returns -1 on error, does not return on success.
 */
long sys_exec(const char *filename, struct trap_frame *tf)
{
    void *elf_data;
    unsigned long elf_size;
    unsigned long entry;
    pagetable_t new_pagetable;
    void *new_stack;
    struct task *task;

    /* Find file in initrd */
    elf_data = initrd_find(filename, &elf_size);
    if (!elf_data) {
        printk("exec: file '%s' not found\n", filename);
        return -1;
    }

    /* Verify ELF header */
    if (elf_verify((const Elf64_Ehdr *)elf_data, elf_size) != 0) {
        printk("exec: invalid ELF file\n");
        return -1;
    }

    /* Create new page table */
    new_pagetable = pgtable_create();
    if (!new_pagetable) {
        printk("exec: out of memory\n");
        return -1;
    }

    /* Map kernel pages */
    if (vm_map_kernel(new_pagetable) != 0) {
        pgtable_free_walk(new_pagetable);
        printk("exec: kernel map failed\n");
        return -1;
    }

    /* Load ELF segments */
    entry = elf_load(new_pagetable, elf_data, elf_size);
    if (entry == 0) {
        pgtable_free_walk(new_pagetable);
        return -1;
    }

    /* Allocate user stack */
    new_stack = page_alloc();
    if (!new_stack) {
        pgtable_free_walk(new_pagetable);
        printk("exec: out of memory\n");
        return -1;
    }

    mem_zero(new_stack, PAGE_SIZE);

    if (pgtable_map(new_pagetable,
                    USER_STACK_TOP - PAGE_SIZE,
                    (unsigned long)new_stack,
                    PAGE_SIZE,
                    PAGE_USER | PAGE_WRITE) != 0) {
        page_free(new_stack);
        pgtable_free_walk(new_pagetable);
        printk("exec: stack map failed\n");
        return -1;
    }

    /* Get current task */
    task = task_current();
    if (!task) {
        page_free(new_stack);
        pgtable_free_walk(new_pagetable);
        return -1;
    }

    /* Free old pages (except kernel stack) */
    /* Note: For simplicity, we leak old pages here. A real OS would free them. */

    /* Update task */
    task->pagetable = new_pagetable;
    task->entry = entry;
    task->user_sp = USER_STACK_TOP;
    task->user_image_size = 0; /* We don't track this for exec'd programs */

    /* Update trap frame */
    tf->sepc = entry;
    tf->sp = USER_STACK_TOP;
    tf->a0 = 0;
    tf->a1 = 0;
    tf->a2 = 0;

    /* Switch to new page table */
    vm_switch(new_pagetable);

    printk("exec: loaded '%s' entry=0x%lx\n", filename, entry);

    return 0; /* Not actually reached */
}

#include <kernel/initrd.h>
#include <kernel/printk.h>
#include <kernel/kmalloc.h>

/* 
 * CPIO newc header format - 110 bytes total
 * All numeric fields are 8-character hex strings
 */
#define CPIO_HEADER_SIZE 110

/* File entry for lookup */
struct initrd_entry {
    char name[64];
    unsigned long data;
    unsigned long size;
    struct initrd_entry *next;
};

static struct initrd_entry *file_list = 0;
static unsigned long initrd_start = 0;
static unsigned long initrd_end = 0;

/* Parse hex string to unsigned long */
static unsigned long parse_hex(const char *s, int len)
{
    unsigned long val = 0;
    int i;

    for (i = 0; i < len; i++) {
        char c = s[i];
        val <<= 4;
        if (c >= '0' && c <= '9') {
            val |= (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            val |= (c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            val |= (c - 'A' + 10);
        }
    }

    return val;
}

/* Align value up to 4 bytes */
static unsigned long align4(unsigned long val)
{
    return (val + 3) & ~3UL;
}

/* Initialize initrd from memory */
void initrd_init(unsigned long start, unsigned long size)
{
    unsigned long pos;
    unsigned long namesize, filesize;
    char *name;
    struct initrd_entry *entry;
    int file_count = 0;

    initrd_start = start;
    initrd_end = start + size;
    file_list = 0;

    printk("initrd: start=0x%lx size=0x%lx\n", start, size);

    pos = start;

    while (pos + CPIO_HEADER_SIZE <= initrd_end) {
        char *hdr = (char *)pos;

        /* Check for end marker "TRAILER!!!" */
        if (hdr[0] == 'T' && hdr[1] == 'R' && hdr[2] == 'A' &&
            hdr[3] == 'I' && hdr[4] == 'L') {
            printk("initrd: found trailer at 0x%lx\n", pos);
            break;
        }

        /* Verify magic "070701" */
        if (hdr[0] != '0' || hdr[1] != '7' || hdr[2] != '0' ||
            hdr[3] != '7' || hdr[4] != '0' || hdr[5] != '1') {
            printk("initrd: bad magic at 0x%lx\n", pos);
            break;
        }

        /* Parse header fields */
        namesize = parse_hex(hdr + 94, 8);  /* c_namesize at offset 94 */
        filesize = parse_hex(hdr + 54, 8);  /* c_filesize at offset 54 */

        printk("initrd: entry at 0x%lx namesize=%lu filesize=%lu\n", 
               pos, namesize, filesize);

        /* Get filename - right after header */
        name = (char *)(pos + CPIO_HEADER_SIZE);
        
        /* Skip "." entry */
        if (name[0] == '.' && name[1] == '\0') {
            printk("initrd: skipping '.' entry\n");
            /* Skip to next entry: header + aligned name + aligned data */
            pos = align4(pos + CPIO_HEADER_SIZE + namesize);
            pos = align4(pos + filesize);
            continue;
        }

        /* Skip if filename is too long */
        if (namesize >= 64) {
            printk("initrd: skipping long name (%lu bytes)\n", namesize);
            pos = align4(pos + CPIO_HEADER_SIZE + namesize);
            pos = align4(pos + filesize);
            continue;
        }

        /* Allocate entry */
        entry = (struct initrd_entry *)kmalloc(sizeof(struct initrd_entry));
        if (!entry) {
            printk("initrd: out of memory\n");
            break;
        }

        /* Copy filename (without trailing NUL in cpio) */
        for (unsigned long i = 0; i < namesize && i < 63; i++) {
            entry->name[i] = name[i];
        }
        entry->name[namesize < 64 ? namesize : 63] = '\0';

        /* Data starts after aligned filename */
        entry->data = align4(pos + CPIO_HEADER_SIZE + namesize);
        entry->size = filesize;

        /* Add to list */
        entry->next = file_list;
        file_list = entry;
        file_count++;

        printk("initrd: file '%s' size=%lu at 0x%lx\n", 
               entry->name, entry->size, entry->data);

        /* Move to next entry: header + aligned name + aligned data */
        pos = align4(entry->data + filesize);
    }

    printk("initrd: loaded %d files\n", file_count);
}

/* Find a file in initrd */
void *initrd_find(const char *name, unsigned long *size)
{
    struct initrd_entry *entry = file_list;

    while (entry) {
        /* Simple string compare */
        int match = 1;
        const char *a = entry->name;
        const char *b = name;
        
        while (*a && *b) {
            if (*a != *b) {
                match = 0;
                break;
            }
            a++;
            b++;
        }
        if (*a != *b) match = 0;

        if (match) {
            if (size) {
                *size = entry->size;
            }
            return (void *)entry->data;
        }

        entry = entry->next;
    }

    return 0;
}

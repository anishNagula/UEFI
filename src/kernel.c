#include <stdint.h>

#include "bootinfo.h"
#include "memory_map.h"
#include "physical.h"
#include "mmu.h"
#include "page_table.h"

void kernel_main(BootInfo *boot_info)
{
    memory_map_init(boot_info);

    physical_memory_init(boot_info);

    uint64_t page1 = physical_alloc_page();
    uint64_t page2 = physical_alloc_page();

    if (page1 == 0 || page2 == 0)
    {
        while (1)
            __asm__ volatile("wfe");
    }

    physical_free_page(page1);
    physical_free_page(page2);

    /*
     * Test creation of an independent page-table hierarchy.
     */
    uint64_t test_root =
        page_table_create();

    if (test_root == 0)
    {
        while (1)
            __asm__ volatile("wfe");
    }

    /*
     * Test a mapping.
     */
    if (page_table_map(
            test_root,
            0x50000000,
            0x50000000,
            PAGE_READ |
            PAGE_WRITE
        ) != 0)
    {
        while (1)
            __asm__ volatile("wfe");
    }

    /*
     * Test removing it.
     */
    if (page_table_unmap(
            test_root,
            0x50000000
        ) != 0)
    {
        while (1)
            __asm__ volatile("wfe");
    }

    /*
     * Existing known-good MMU implementation.
     */
    mmu_init(
        boot_info->kernel_base,
        boot_info->kernel_size
    );

    while (1)
    {
        __asm__ volatile("wfe");
    }
}
#include <stdint.h>

#include "bootinfo.h"
#include "memory_map.h"
#include "physical.h"

void kernel_main(BootInfo *boot_info)
{
    memory_map_init(boot_info);
    physical_memory_init(boot_info);

    /*
     * Basic physical allocator smoke test.
     */
    uint64_t page1 = physical_alloc_page();
    uint64_t page2 = physical_alloc_page();

    if (page1 == 0 || page2 == 0)
    {
        while (1)
        {
            __asm__ volatile("wfe");
        }
    }

    physical_free_page(page1);
    physical_free_page(page2);

    while (1)
    {
        __asm__ volatile("wfe");
    }
}
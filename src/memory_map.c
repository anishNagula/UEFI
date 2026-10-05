#include "memory_map.h"

#define PAGE_SIZE 4096ULL

/*
 * UEFI EFI_MEMORY_DESCRIPTOR
 *
 * The firmware reports a descriptor size of 0x30 on QEMU.
 *
 * We only use the fields through Attribute, but the complete
 * layout is represented so that the offsets are explicit.
 */
typedef struct
{
    uint32_t type;
    uint32_t reserved;

    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t number_of_pages;
    uint64_t attribute;

    uint64_t reserved2;
} EfiMemoryDescriptor;

static BootInfo *memory_boot_info;

static uint64_t max_address;
static uint64_t usable_memory;

void memory_map_init(BootInfo *boot_info)
{
    memory_boot_info = boot_info;

    max_address = 0;
    usable_memory = 0;

    uint64_t map =
        boot_info->memory_map;

    uint64_t map_end =
        map + boot_info->memory_map_size;

    while (map < map_end)
    {
        EfiMemoryDescriptor *desc =
            (EfiMemoryDescriptor *)(uintptr_t)map;

        uint64_t size =
            desc->number_of_pages * PAGE_SIZE;

        uint64_t end =
            desc->physical_start + size;

        if (end > max_address)
            max_address = end;

        if (desc->type == EFI_CONVENTIONAL_MEMORY)
            usable_memory += size;

        map += boot_info->memory_descriptor_size;
    }
}

uint64_t memory_map_max_address(void)
{
    return max_address;
}

uint64_t memory_map_usable_memory(void)
{
    return usable_memory;
}

int memory_map_is_usable(uint64_t address)
{
    uint64_t map =
        memory_boot_info->memory_map;

    uint64_t map_end =
        map + memory_boot_info->memory_map_size;

    while (map < map_end)
    {
        EfiMemoryDescriptor *desc =
            (EfiMemoryDescriptor *)(uintptr_t)map;

        if (desc->type == EFI_CONVENTIONAL_MEMORY)
        {
            uint64_t start =
                desc->physical_start;

            uint64_t end =
                start +
                desc->number_of_pages * PAGE_SIZE;

            if (address >= start && address < end)
                return 1;
        }

        map += memory_boot_info->memory_descriptor_size;
    }

    return 0;
}
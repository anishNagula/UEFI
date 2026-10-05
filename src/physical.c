#include "physical.h"
#include "memory_map.h"

#define BITS_PER_BYTE 8ULL

/*
 * UEFI EFI_MEMORY_DESCRIPTOR.
 *
 * The firmware descriptor size is supplied through BootInfo,
 * so we advance through the map using that stride.
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

static uint8_t *bitmap;

static uint64_t total_pages;
static uint64_t free_pages;

static uint64_t bitmap_start;
static uint64_t bitmap_end;

static inline uint64_t align_up(uint64_t value)
{
    return (value + PAGE_SIZE - 1) &
           ~(PAGE_SIZE - 1);
}

static inline void bitmap_set(uint64_t page)
{
    bitmap[page / BITS_PER_BYTE] |=
        (uint8_t)(1ULL << (page % BITS_PER_BYTE));
}

static inline void bitmap_clear(uint64_t page)
{
    bitmap[page / BITS_PER_BYTE] &=
        (uint8_t)~(1ULL << (page % BITS_PER_BYTE));
}

static inline int bitmap_test(uint64_t page)
{
    return
        (bitmap[page / BITS_PER_BYTE] >>
         (page % BITS_PER_BYTE)) & 1;
}

void physical_memory_init(BootInfo *boot_info)
{
    uint64_t max_address =
        memory_map_max_address();

    total_pages =
        max_address / PAGE_SIZE;

    /*
     * One bit per physical page.
     */
    uint64_t bitmap_size =
        (total_pages + 7) / 8;

    /*
     * The bitmap itself needs whole pages.
     */
    uint64_t bitmap_pages =
        (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;

    /*
     * Find a conventional-memory region large enough
     * to hold the bitmap.
     */
    uint64_t map =
        boot_info->memory_map;

    uint64_t map_end =
        map + boot_info->memory_map_size;

    bitmap_start = 0;
    bitmap_end = 0;

    while (map < map_end)
    {
        EfiMemoryDescriptor *desc =
            (EfiMemoryDescriptor *)(uintptr_t)map;

        if (desc->type == EFI_CONVENTIONAL_MEMORY)
        {
            uint64_t region_start =
                align_up(desc->physical_start);

            uint64_t region_end =
                desc->physical_start +
                desc->number_of_pages * PAGE_SIZE;

            /*
             * Make sure the bitmap region does not overlap
             * the kernel.
             */
            uint64_t kernel_start =
                boot_info->kernel_base;

            uint64_t kernel_end =
                align_up(
                    boot_info->kernel_base +
                    boot_info->kernel_size
                );

            uint64_t candidate =
                region_start;

            if (candidate < kernel_end &&
                region_end > kernel_start)
            {
                candidate = kernel_end;
            }

            candidate = align_up(candidate);

            if (candidate >= region_start &&
                candidate < region_end)
            {
                uint64_t available_pages =
                    (region_end - candidate) / PAGE_SIZE;

                if (available_pages >= bitmap_pages)
                {
                    bitmap_start = candidate;
                    bitmap_end =
                        candidate +
                        bitmap_pages * PAGE_SIZE;

                    break;
                }
            }
        }

        map += boot_info->memory_descriptor_size;
    }

    /*
     * If we couldn't find somewhere safe for the bitmap,
     * stop here.
     */
    if (bitmap_start == 0)
    {
        while (1)
        {
            __asm__ volatile("wfe");
        }
    }

    bitmap =
        (uint8_t *)(uintptr_t)bitmap_start;

    /*
     * Initially mark every physical page as USED.
     */
    for (uint64_t i = 0;
         i < bitmap_size;
         i++)
    {
        bitmap[i] = 0xFF;
    }

    free_pages = 0;

    /*
     * Walk the UEFI memory map and mark conventional
     * memory as free.
     */
    map = boot_info->memory_map;

    while (map < map_end)
    {
        EfiMemoryDescriptor *desc =
            (EfiMemoryDescriptor *)(uintptr_t)map;

        if (desc->type == EFI_CONVENTIONAL_MEMORY)
        {
            uint64_t start =
                align_up(desc->physical_start);

            uint64_t end =
                desc->physical_start +
                desc->number_of_pages * PAGE_SIZE;

            for (uint64_t address = start;
                 address < end;
                 address += PAGE_SIZE)
            {
                uint64_t page =
                    address / PAGE_SIZE;

                if (page >= total_pages)
                    continue;

                if (bitmap_test(page))
                {
                    bitmap_clear(page);
                    free_pages++;
                }
            }
        }

        map += boot_info->memory_descriptor_size;
    }

    /*
     * Reserve the kernel.
     */
    uint64_t kernel_start =
        boot_info->kernel_base;

    uint64_t kernel_end =
        align_up(
            boot_info->kernel_base +
            boot_info->kernel_size
        );

    for (uint64_t address = kernel_start;
         address < kernel_end;
         address += PAGE_SIZE)
    {
        uint64_t page =
            address / PAGE_SIZE;

        if (page < total_pages &&
            !bitmap_test(page))
        {
            bitmap_set(page);
            free_pages--;
        }
    }

    /*
     * Reserve the bitmap.
     */
    for (uint64_t address = bitmap_start;
         address < bitmap_end;
         address += PAGE_SIZE)
    {
        uint64_t page =
            address / PAGE_SIZE;

        if (page < total_pages &&
            !bitmap_test(page))
        {
            bitmap_set(page);
            free_pages--;
        }
    }
}

uint64_t physical_alloc_page(void)
{
    for (uint64_t page = 0;
         page < total_pages;
         page++)
    {
        if (!bitmap_test(page))
        {
            bitmap_set(page);
            free_pages--;

            return page * PAGE_SIZE;
        }
    }

    return 0;
}

void physical_free_page(uint64_t address)
{
    if (address == 0)
        return;

    if (address % PAGE_SIZE != 0)
        return;

    /*
     * Never free the bitmap.
     */
    if (address >= bitmap_start &&
        address < bitmap_end)
        return;

    uint64_t page =
        address / PAGE_SIZE;

    if (page >= total_pages)
        return;

    if (!bitmap_test(page))
        return;

    bitmap_clear(page);
    free_pages++;
}

uint64_t physical_total_pages(void)
{
    return total_pages;
}

uint64_t physical_free_pages(void)
{
    return free_pages;
}
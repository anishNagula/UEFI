#include <stdint.h>

#include "mmu.h"
#include "physical.h"

#define PAGE_SIZE 4096ULL

#define TABLE_ENTRIES 512

#define PTE_VALID       (1ULL << 0)
#define PTE_TABLE       (1ULL << 1)

#define PTE_AF          (1ULL << 10)

// AttrIndx = 0, MAIR_EL1[7:0] will describe normal memory.
#define PTE_ATTR_NORMAL (0ULL << 2)

// Sharability, 11 = Inner Shareable
#define PTE_INNER_SHARABLE (3ULL << 8)

// Access permissions, AP[2:1] = 00, EL1 read/write.
#define PTE_AP_RW_EL1 (0ULL << 6)

//2 MiB block descriptor.
#define PTE_BLOCK (0ULL << 1)

static uint64_t *l1_table;
static uint64_t *l2_table;

static void zero_page(uint64_t address) {
    uint64_t *page = (uint64_t *)(uintptr_t)address;

    for (uint64_t i = 0; i < PAGE_SIZE / sizeof(uint64_t); i++) {
        page[i] = 0;
    }
}

static uint64_t alloc_table(void) {
    uint64_t page = physical_alloc_page();

    if (page == 0) {
        while (1) {
            __asm__ volatile("wfe");
        }
    }

    zero_page(page);
    return page;
}

static void configure_mair(void) {
    /*
     * MAIR:
     *
     * Attr0 = 0xFF
     *
     * Normal memory:
     * Inner/Outer Write-back cacheable.
     */

    uint64_t mair = 0xFFULL;

    __asm__ volatile(
        "msr mair_el1, %0"
        :
        : "r"(mair)
    );

    __asm__ volatile("isb");
}

static void configure_tcr(void) {
    /*
     * 4 KiB granule.
     *
     * T0SZ = 25
     *
     * Gives a 39-bit VA space:
     *
     * 64 - 25 = 39 bits.
     *
     * TG0 = 00
     *
     * 4 KiB granule.
     *
     * SH0 = 11
     *
     * Inner Shareable.
     *
     * ORGN0/IRGN0 = 01
     *
     * Write-back cacheable.
     */

    uint64_t tcr = (25ULL << 0) | (1ULL << 8) | (1ULL << 10) | (3ULL << 12);

    __asm__ volatile(
        "msr tcr_el1, %0"
        :
        : "r"(tcr)
    );

    __asm__ volatile("isb");
}

static void map_2mb_block(uint64_t *table, uint64_t index, uint64_t physical) {
    table[index] = (physical & 0x0000FFFFFFFFE000ULL) |
                    PTE_VALID |
                    PTE_AF |
                    PTE_ATTR_NORMAL |
                    PTE_INNER_SHARABLE |
                    PTE_AP_RW_EL1 |
                    PTE_BLOCK;
}

void mmu_init(uint64_t kernel_base, uint64_t kernel_size) {
    configure_mair();
    configure_tcr();

    uint64_t l1 = alloc_table();
    uint64_t l2 = alloc_table();

    l1_table = (uint64_t *)(uintptr_t)l1;
    l2_table = (uint64_t *)(uintptr_t)l2;

    l1_table[0] = (l2 & ~0xFFFULL) | PTE_VALID | PTE_TABLE;

    for (uint64_t i = 0; i < TABLE_ENTRIES; i++) {
        uint64_t physical = i * 0x200000ULL;

        map_2mb_block(
            l2_table,
            i,
            physical
        );
    }

    (void)kernel_base;
    (void)kernel_size;

    extern void mmu_enable(uint64_t table);

    mmu_enable(l1);
}
#include <stdint.h>

#include "page_table.h"
#include "physical.h"

#define PTE_VALID        (1ULL << 0)
#define PTE_TABLE        (1ULL << 1)

#define PTE_AF           (1ULL << 10)

#define PTE_SH_INNER     (3ULL << 8)

#define PTE_AP_RW_EL1    (0ULL << 6)

#define PTE_ATTR_NORMAL  (0ULL << 2)

/*
 * MAIR index 1 will be device memory.
 */
#define PTE_ATTR_DEVICE  (1ULL << 2)

#define PTE_XN           (1ULL << 54)

static void zero_page(uint64_t address) {
    uint64_t *ptr = (uint64_t *)(uintptr_t)address;

    for (uint64_t i = 0; i < PAGE_TABLE_ENTRIES; i++) {
        ptr[i] = 0;
    }
}

static uint64_t allocate_table(void) {
    uint64_t page = physical_alloc_page();

    if (page == 0) {
        return 0;
    }

    zero_page(page);

    return page;
}

static uint64_t table_entry(uint64_t table, uint64_t index) {
    uint64_t *entries = (uint64_t *)(uintptr_t)table;

    return entries[index];
}

static void set_table_entry(uint64_t table, uint64_t index, uint64_t value) {
    uint64_t *entries = (uint64_t *)(uintptr_t)table;

    entries[index] = value;
}

static uint64_t entry_address(uint64_t entry) {
    return entry & 0x0000FFFFFFFFF000ULL;
}

static int entry_valid(uint64_t entry) {
    return (entry & PTE_VALID) != 0;
}

static uint64_t level_index(uint64_t virtual_address, int level) {
    switch(level) {
        case 1:
            return (virtual_address >> 30) & 0x1FF;
        case 2:
            return (virtual_address >> 21) & 0x1FF;
        case 3:
            return (virtual_address >> 12) & 0x1FF;
        default:
            return 0;
    }
}

uint64_t page_table_create(void) {
    return allocate_table();
}

int page_table_map(uint64_t root, uint64_t virtual_address, uint64_t physical_address, uint64_t flags) {
    if (root == 0) return -1;

    if ((virtual_address & 0xFFF) != 0) return -1;

    if ((virtual_address & 0xFFF) != 0) return -1;

    uint64_t current = root;

    for (int level = 1; level <= 3; level++) {
        uint64_t index = level_index(virtual_address, level);
        uint64_t entry = table_entry(current, index);

        if (!entry_valid(entry)) {
            uint64_t next = allocate_table();
            
            if (next == 0) return -1;

            set_table_entry(current, index, next | PTE_VALID | PTE_TABLE);
            
            current = next;
        } else {
            if (!(entry & PTE_TABLE)) return -1;
            current = entry_address(entry);
        }
    }

    uint64_t index = level_index(virtual_address, 3);

    uint64_t entry_flags = PTE_VALID | PTE_AF | PTE_SH_INNER;

    if (flags & PAGE_DEVICE) {
        entry_flags |= PTE_ATTR_DEVICE;
    } else {
        entry_flags |= PTE_ATTR_NORMAL;
    }

    if (!(flags & PAGE_WRITE)) {
         /*
         * AP[2:1] = 10 would mean read-only.
         *
         * We keep EL1 read/write for now because
         * this is our initial kernel address space.
         */
    }
    
    if (!(flags & PAGE_EXECUTE)) {
        entry_flags |= PTE_XN;
    }

    set_table_entry(current, index, (physical_address & 0x0000FFFFFFFFF000ULL) | entry_flags);

    return 0;
}

int page_table_unmap(uint64_t root, uint64_t virtual_address) {
    if (root == 0) return -1;

    if (virtual_address & 0xFFF) return -1;

    uint64_t current = root;

    for (int level = 1; level <= 3; level++) {
        uint64_t index = level_index(virtual_address, level);
        uint64_t entry = table_entry(current, index);

        if (!entry_valid(entry)) return -1;

        if (!(entry & PTE_TABLE)) return -1;

        current = entry_address(entry);
    }

    uint64_t index = level_index(virtual_address, 3);
    uint64_t entry = table_entry(current, index);

    if (!entry_valid(entry)) return -1;

    set_table_entry(current, index, 0);

    return 0;
}
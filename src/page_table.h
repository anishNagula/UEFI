#ifndef SNOOPY_PAGE_TABLE_H
#define SNOOPY_PAGE_TABLE_H

#include <stdint.h>

#define PAGE_TABLE_ENTRIES 512

#define PAGE_TABLE_LEVELS 4

#define PAGE_TABLE_PAGE_SIZE 4096ULL

/*
 * Mapping flags.
 */
#define PAGE_READ       (1ULL << 0)
#define PAGE_WRITE      (1ULL << 1)
#define PAGE_EXECUTE    (1ULL << 2)
#define PAGE_DEVICE     (1ULL << 3)

uint64_t page_table_create(void);

int page_table_map(uint64_t root, uint64_t virtual_address, uint64_t physical_address, uint64_t flags);

int page_table_unmap(uint64_t root, uint64_t virtual_address);

#endif
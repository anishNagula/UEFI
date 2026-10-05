#ifndef SNOOPY_PHYSICAL_H
#define SNOOPY_PHYSICAL_H

#include <stdint.h>
#include "bootinfo.h"

#define PAGE_SIZE 4096ULL

void physical_memory_init(BootInfo *boot_info);

uint64_t physical_alloc_page(void);

void physical_free_page(uint64_t address);

uint64_t physical_total_pages(void);

uint64_t physical_free_pages(void);

#endif
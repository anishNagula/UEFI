#ifndef SNOOPY_MEMORY_MAP_H
#define SNOOPY_MEMORY_MAP_H

#include <stdint.h>
#include "bootinfo.h"


#define EFI_CONVENTIONAL_MEMORY 7

typedef struct {
    uint64_t base;
    uint64_t size;
} MemoryRegion;


void memory_map_init(BootInfo *boot_info);

uint64_t memory_map_max_address(void);

uint64_t memory_map_usable_memory(void);

int memory_map_is_usable(uint64_t address);

#endif
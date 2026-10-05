#ifndef SNOOPY_BOOTINFO_H
#define SNOOPY_BOOTINFO_H

#include <stdint.h>

typedef struct
{
    uint64_t memory_map;
    uint64_t memory_map_size;
    uint64_t memory_descriptor_size;

    uint64_t kernel_base;
    uint64_t kernel_size;

} BootInfo;

#endif
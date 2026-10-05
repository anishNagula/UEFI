#ifndef SNOOPY_MMU_H
#define SNOOPY_MMU_H

#include <stdint.h>

void mmu_init(uint64_t kernel_base, uint64_t kernel_size);

#endif
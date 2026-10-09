#ifndef SNOOPY_MMU_H
#define SNOOPY_MMU_H

#include <stdint.h>

#define PAGE_SIZE 4096ULL

#define MMU_PAGE_READ        (1ULL << 0)
#define MMU_PAGE_WRITE       (1ULL << 1)
#define MMU_PAGE_EXECUTE     (1ULL << 2)
#define MMU_PAGE_DEVICE      (1ULL << 3)

void mmu_init(uint64_t kernel_base, uint64_t kernel_size);

#endif
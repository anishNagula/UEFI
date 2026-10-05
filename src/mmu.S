.section .text

.global mmu_enable
.type mmu_enable, %function

// x0 = address of translation table used by TTBR0_EL1

mmu_enable:
    // install translation table
    msr ttbr0_el1, x0

    // invalidate any stale TLB entries
    dsb ish
    tlbi vmalle1
    dsb ish
    isb

    // enable mmu and caches, M = bit 0, C = bit 2, I = bit 12
    mrs x1, sctlr_el1

    orr x1, x1, #(1 << 0)
    orr x1, x1, #(1 << 2)
    orr x1, x1, #(1 << 12)

    msr sctlr_el1, x1
    isb

    ret

.size mmu_enable, . - mmu_enable
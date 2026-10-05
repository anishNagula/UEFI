.section .text.vectors
.align 11
.global exception_vectors

exception_vectors:

    // current EL with SP0
    b sync_sp0
    .align 7
    b irq_sp0
    .align 7
    b fiq_sp0
    .align 7
    b serror_sp0
    .align 7

    // current EL with SPx
    b sync_spx
    .align 7
    b irq_spx
    .align 7
    b fiq_spx
    .align 7
    b serror_spx
    .align 7

    // lower EL using AArch64
    b sync_lower_a64
    .align 7
    b irq_lower_a64
    .align 7
    b fiq_lower_a64
    .align 7
    b serror_lower_a64
    .align 7

    // lower EL using AArch32
    b sync_lower_a32
    .align 7
    b irq_lower_a32
    .align 7
    b fiq_lower_a32
    .align 7
    b serror_lower_a32
    .align 7

sync_sp0:
sync_spx:
sync_lower_a64:
sync_lower_a32:
    b exception_sync

irq_sp0:
irq_spx:
irq_lower_a64:
irq_lower_a32:
    b exception_irq

fiq_sp0:
fiq_spx:
fiq_lower_a64:
fiq_lower_a32:
    b exception_fiq

serror_sp0:
serror_spx:
serror_lower_a64:
serror_lower_a32:
    b exception_serror
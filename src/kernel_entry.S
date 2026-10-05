.section .text.entry

.global kernel_entry
.global kernel_stack
.global kernel_stack_top

.extern kernel_main
.extern exception_vectors

.type kernel_entry, %function

kernel_entry:

    /*
     * x0 already contains BootInfo*.
     *
     * Do not overwrite x0.
     */

    /*
     * Establish our own kernel stack.
     */
    ldr x1, =kernel_stack_top
    mov sp, x1

    /*
     * Install exception vector table.
     */
    ldr x1, =exception_vectors
    msr vbar_el1, x1
    isb

    /*
     * Call:
     *
     *     kernel_main(BootInfo *)
     *
     * x0 contains the argument.
     */
    bl kernel_main

1:
    wfe
    b 1b

.size kernel_entry, . - kernel_entry


.section .bss

.align 12

.global kernel_stack

kernel_stack:
    .skip 16384

.global kernel_stack_top

kernel_stack_top:
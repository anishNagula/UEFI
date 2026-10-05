#define UART_BASE 0x09000000UL

static volatile unsigned int *const UART = (volatile unsigned int *) UART_BASE;

static void uart_putc(char c) {
    *UART = (unsigned int)c;
}

static void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

static void uart_hex(unsigned long value) {
    const char *digits = "0123456789ABCDEF";

    uart_puts("0x");

    for (int i = 15; i >= 0; i--) {
        uart_putc(digits[(value >> (i * 4)) & 0xF]);
    }
}

static unsigned long read_esr(void) {
    unsigned long value;

    __asm__ volatile(
        "mrs %0, esr_el1"
        : "=r"(value)
    );

    return value;
}

static unsigned long read_elr(void) {
    unsigned long value;

    __asm__ volatile(
        "mrs %0, elr_el1"
        : "=r"(value)
    );

    return value;
}

static unsigned long read_far(void) {
    unsigned long value;

    __asm__ volatile(
        "mrs %0, far_el1"
        : "=r"(value)
    );
    
    return value;
}

void exception_sync(void) {
    unsigned long esr = read_esr();
    unsigned long elr = read_elr();
    unsigned long far = read_far();

    uart_puts("\r\n");
    uart_puts("================================\r\n");
    uart_puts("       SNOOPY EXCEPTION          \r\n");
    uart_puts("================================\r\n");

    uart_puts("ESR_EL1: ");
    uart_hex(esr);
    uart_puts("\r\n");

    uart_puts("ELR_EL1: ");
    uart_hex(elr);
    uart_puts("\r\n");

    uart_puts("FAR_EL1: ");
    uart_hex(far);
    uart_puts("\r\n");

    uart_puts("System halted.\r\n");

    while (1) {
        __asm__ volatile("wfe");
    }
}

void exception_irq(void) {
    uart_puts("\r\nSNOOPY IRQ\r\n");

    while (1) {
        __asm__ volatile("wfe");
    }
}

void exception_fiq(void) {
    uart_puts("\r\nSNOOPY FIQ\r\n");

    while (1) {
        __asm__ volatile("wfe");
    }
}

void exception_serror(void) {
    uart_puts("\r\nSNOOPY SError\r\n");

    while (1) {
        __asm__ volatile("wfe");
    }
}
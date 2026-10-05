#include <stdint.h>

#include "bootinfo.h"

#define UART_BASE 0x09000000UL

static volatile uint32_t *const UART =
    (volatile uint32_t *)UART_BASE;


static void uart_putc(char c)
{
    *UART = (uint32_t)c;
}


static void uart_puts(const char *s)
{
    while (*s)
    {
        uart_putc(*s++);
    }
}


static void uart_hex(uint64_t value)
{
    const char *digits =
        "0123456789ABCDEF";

    uart_puts("0x");

    for (int i = 15; i >= 0; i--)
    {
        uart_putc(
            digits[
                (value >> (i * 4)) & 0xF
            ]
        );
    }
}


void kernel_main(BootInfo *boot_info)
{
    uart_puts("\r\n");
    uart_puts("================================\r\n");
    uart_puts("       SNOOPY KERNEL ONLINE      \r\n");
    uart_puts("================================\r\n");

    uart_puts(
        "Kernel stack initialized.\r\n"
    );

    uart_puts(
        "Kernel received BootInfo.\r\n"
    );

    uart_puts(
        "Memory map address: "
    );

    uart_hex(
        boot_info->memory_map
    );

    uart_puts("\r\n");

    uart_puts(
        "Memory map size: "
    );

    uart_hex(
        boot_info->memory_map_size
    );

    uart_puts("\r\n");

    uart_puts(
        "Memory descriptor size: "
    );

    uart_hex(
        boot_info->memory_descriptor_size
    );

    uart_puts("\r\n");

    uart_puts(
        "Kernel base: "
    );

    uart_hex(
        boot_info->kernel_base
    );

    uart_puts("\r\n");

    uart_puts(
        "Kernel size: "
    );

    uart_hex(
        boot_info->kernel_size
    );

    uart_puts("\r\n");

    uart_puts(
        "Boot information handoff successful.\r\n"
    );


    while (1)
    {
        __asm__ volatile("wfe");
    }
}
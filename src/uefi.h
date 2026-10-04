#ifndef SNOOPY_UEFI_H
#define SNOOPY_UEFI_H

#include <stdint.h>

typedef uint64_t EFI_STATUS;
typedef void *EFI_HANDLE;

#define EFI_SUCCESS 0

typedef struct {
    uint64_t Signature;
    uint32_t Revision;
    uint32_t HeaderSize;
    uint32_t CRC32;
    uint32_t Reserved;
} EFI_TABLE_HEADER;

typedef struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef EFI_STATUS (*EFI_TEXT_STRING)(
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *this,
    const uint16_t *String
);

struct EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void *Reset;
    EFI_TEXT_STRING OutputString;
    void *TestString;
    void *QueryMode;
    void *SetMode;
    void *setAttribute;
    void *ClearScreen;
    void *SetCursorPosition;
    void *EnableCursor;
    void *Mode;
};

typedef struct {
    EFI_TABLE_HEADER Header;

    uint16_t *FirmwareVendor;
    uint32_t *FirmwareRevision;

    EFI_HANDLE ConsoleInHandle;
    void *ConIn;

    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    
    EFI_HANDLE StandardErrorHandle;
    void *StdErr;

    void *RuntimesServices;
    void *BootServices;

    uint64_t NumberOfTableEntries;
    void *ConfigurationTable;
} EFI_SYSTEM_TABLE;

#endif
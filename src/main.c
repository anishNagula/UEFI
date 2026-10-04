#include <efi.h>
#include <efilib.h>

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    InitializeLib(ImageHandle, SystemTable);

    Print(L"\r\n");
    Print(L"===============================================\r\n");
    Print(L"        Snoopy AArch64 UEFI Loader             \r\n");
    Print(L"===============================================\r\n");
    Print(L"\r\n");

    Print(L"GNU-EFI initialized successfully!\r\n");
    Print(L"Using the real UEFI headers and library.\r\n");
    Print(L"\r\n");

    while (1) {
        __asm__ volatile("wfe");
    }

    return EFI_SUCCESS;
}
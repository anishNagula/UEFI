#include "uefi.h"

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *st) {
    (void)image;

    st->ConOut->OutputString(
        st->ConOut,
        L"\r\n"
        L"===============================================\r\n"
        L"        Snoopy AArch64 UEFI Loader             \r\n"
        L"===============================================\r\n"
        L"\r\n"
        L"UEFI successfully handed control to Snoopy!\r\n"
        L"\r\n"
    );

    while (1) {
        __asm__ volatile("wfe");
    }

    return EFI_SUCCESS;
}
#include <efi.h>
#include <efilib.h>

#define ELF_MAGIC0 0x7f
#define ELF_MAGIC1 'E'
#define ELF_MAGIC2 'L'
#define ELF_MAGIC3 'F'

#define ELFCLASS64 2
#define ELFDATA2LSB 1

#define ET_EXEC 2
#define EM_AARCH64 183

#define PT_LOAD 1

typedef struct {
    UINT8 e_ident[16];
    UINT16 e_type;
    UINT16 e_machine;
    UINT32 e_version;
    UINT64 e_entry;
    UINT64 e_phoff;
    UINT64 e_shoff;
    UINT32 e_flags;
    UINT16 e_ehsize;
    UINT16 e_phentsize;
    UINT16 e_phnum;
    UINT16 e_shentsize;
    UINT16 e_shnum;
    UINT16 e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    UINT32 p_type;
    UINT32 p_flags;
    UINT64 p_offset;
    UINT64 p_vaddr;
    UINT64 p_paddr;
    UINT64 p_filesz;
    UINT64 p_memsz;
    UINT64 p_align;
} Elf64_Phdr;


EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status;

    InitializeLib(ImageHandle, SystemTable);

    Print(L"\r\n");
    Print(L"===============================================\r\n");
    Print(L"        Snoopy AArch64 UEFI Loader             \r\n");
    Print(L"===============================================\r\n");
    

    EFI_HANDLE *Handles = NULL;
    UINTN HandleCount = 0;

    Status = LibLocateHandle(
        ByProtocol,
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        &HandleCount,
        &Handles
    );

    if (EFI_ERROR(Status)) {
        Print(L"Failed to locate EFI filesystems: %r\r\n", Status);
        return Status;
    }

    Print(L"Filesystem handles found: %u\r\n", HandleCount);

    EFI_FILE_HANDLE Root = NULL;
    EFI_FILE_HANDLE KernelFile = NULL;

    for (UINTN i = 0; i < HandleCount; i++) {
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;

        Status = uefi_call_wrapper(
            BS->HandleProtocol,
            3,
            Handles[i],
            &gEfiSimpleFileSystemProtocolGuid,
            (void **)&FileSystem
        );

        if (EFI_ERROR(Status)) {
            continue;
        }

        Status = uefi_call_wrapper(
            FileSystem->OpenVolume,
            2,
            FileSystem,
            &Root
        );

        if (EFI_ERROR(Status)) {
            continue;
        }

        Status = uefi_call_wrapper(
            Root->Open,
            5,
            Root,
            &KernelFile,
            L"kernel.elf",
            EFI_FILE_MODE_READ,
            0
        );

        if (!EFI_ERROR(Status)) {
            break;
        }

        Root = NULL;
    }

    if (KernelFile == NULL) {
        Print(L"kernel.elf not found!\r\n");
        return EFI_NOT_FOUND;
    }

    Print(L"kernel.elf opened successfully!\r\n\r\n");


    // read elf header

    Elf64_Ehdr ElfHeader;
    UINTN HeaderSize = sizeof(ElfHeader);

    Status = uefi_call_wrapper(
        KernelFile->Read,
        3,
        KernelFile,
        &HeaderSize,
        &ElfHeader
    );

    if (EFI_ERROR(Status) || HeaderSize != sizeof(ElfHeader)) {
        Print(L"Failed to read ELF header: %r\r\n", Status);
        return EFI_LOAD_ERROR;
    }

    Print(L"Validating kernel ELF...\r\n\r\n");

    // validate elf identification

    if (ElfHeader.e_ident[0] != ELF_MAGIC0 ||
        ElfHeader.e_ident[1] != ELF_MAGIC1 ||
        ElfHeader.e_ident[2] != ELF_MAGIC2 ||
        ElfHeader.e_ident[3] != ELF_MAGIC3) {

        Print(L"Invalid ELF magic!\r\n");
        return EFI_LOAD_ERROR;
    }

    if (ElfHeader.e_ident[4] != ELFCLASS64) {
        Print(L"Kernel is not ELF64!\r\n");
        return EFI_LOAD_ERROR;
    }

    if (ElfHeader.e_ident[5] != ELFDATA2LSB) {
        Print(L"Kernel is not little-endian!\r\n");
        return EFI_LOAD_ERROR;
    }

    if (ElfHeader.e_type != ET_EXEC) {
        Print(L"Kernel is not an executable ELF!\r\n");
        return EFI_LOAD_ERROR;
    }

    if (ElfHeader.e_machine != EM_AARCH64) {
        Print(L"Kernel is not AArch64!\r\n");
        return EFI_LOAD_ERROR;
    }



    // print findings

    Print(L"Kernel ELF validated successfully!\r\n\r\n");

    Print(L"Entry point:          0x%lx\r\n", ElfHeader.e_entry);
    Print(L"Program header offset: 0x%lx\r\n", ElfHeader.e_phoff);
    Print(L"Program header size:   0x%x\r\n", ElfHeader.e_phentsize);
    Print(L"Program header count:  %u\r\n", ElfHeader.e_phnum);


    // read program headers
    UINTN ProgramHeaderSize = ElfHeader.e_phentsize * ElfHeader.e_phnum;

    Elf64_Phdr *ProgramHeaders = AllocatePool(ProgramHeaderSize);

    if (ProgramHeaders == NULL) {
        Print(L"Failed to allocate program headers!\r\n");
        return EFI_OUT_OF_RESOURCES;
    }

    Status = uefi_call_wrapper(
        KernelFile->SetPosition,
        2,
        KernelFile,
        ElfHeader.e_phoff
    );

    if (EFI_ERROR(Status)) {
        Print(L"Failed to seek to program headers: %r\r\n", Status);
        FreePool(ProgramHeaders);
        return Status;
    }

    UINTN ReadSize = ProgramHeaderSize;

    Status = uefi_call_wrapper(
        KernelFile->Read,
        3,
        KernelFile,
        &ReadSize,
        ProgramHeaders
    );
    
    if (EFI_ERROR(Status) || ReadSize != ProgramHeaderSize) {
        Print(L"Failed to read program headers: %r\r\n", Status);
        FreePool(ProgramHeaders);
        return EFI_LOAD_ERROR;
    }

    Print(L"\r\nProgram headers:\r\n");

    for (UINTN i = 0; i < ElfHeader.e_phnum; i++) {
        Elf64_Phdr *Phdr = &ProgramHeaders[i];

        Print(
            L"  [%u] type=0x%x offset=0x%lx vaddr=0x%lx filesz=0x%lx memsz=0x%lx\r\n",
            i,
            Phdr->p_type,
            Phdr->p_offset,
            Phdr->p_vaddr,
            Phdr->p_filesz,
            Phdr->p_memsz
        );
    }

    Print(L"\r\nSnoopy successfully parsed kernel.elf!\r\n");
    Print(L"Next Step: load PT_LOAD segments.\r\n");

    FreePool(ProgramHeaders);

    while (1) {
        __asm__ volatile("wfe");
    }

    return EFI_SUCCESS;

}
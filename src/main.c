#include <efi.h>
#include <efilib.h>

#include "bootinfo.h"

#define ELF_MAGIC0 0x7f
#define ELF_MAGIC1 'E'
#define ELF_MAGIC2 'L'
#define ELF_MAGIC3 'F'

#define ELFCLASS64 2
#define ELFDATA2LSB 1

#define ET_EXEC 2
#define EM_AARCH64 183

#define PT_LOAD 1

typedef void (*KernelEntry)(BootInfo *BootInfo);

typedef struct
{
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

typedef struct
{
    UINT32 p_type;
    UINT32 p_flags;

    UINT64 p_offset;
    UINT64 p_vaddr;
    UINT64 p_paddr;

    UINT64 p_filesz;
    UINT64 p_memsz;

    UINT64 p_align;

} Elf64_Phdr;


/*
 * Get the UEFI memory map and exit Boot Services.
 *
 * On success:
 *
 *   OutMemoryMap       -> memory map buffer
 *   OutMemoryMapSize   -> actual size
 *   OutDescriptorSize  -> size of each descriptor
 *
 * The memory map buffer must NOT be freed after
 * ExitBootServices succeeds.
 */
static EFI_STATUS
ExitBootServicesWithRetry(
    EFI_HANDLE ImageHandle,
    EFI_MEMORY_DESCRIPTOR **OutMemoryMap,
    UINTN *OutMemoryMapSize,
    UINTN *OutDescriptorSize
)
{
    EFI_STATUS Status;

    UINTN MemoryMapSize = 0;
    UINTN MapKey = 0;
    UINTN DescriptorSize = 0;
    UINT32 DescriptorVersion = 0;

    /*
     * First call asks UEFI how large the memory map is.
     */
    Status = uefi_call_wrapper(
        BS->GetMemoryMap,
        5,
        &MemoryMapSize,
        NULL,
        &MapKey,
        &DescriptorSize,
        &DescriptorVersion
    );

    if (Status != EFI_BUFFER_TOO_SMALL)
    {
        Print(
            L"GetMemoryMap size query failed: %r\r\n",
            Status
        );

        return Status;
    }

    /*
     * Extra space gives us room for changes to the
     * memory map.
     */
    MemoryMapSize += 2 * DescriptorSize;

    EFI_MEMORY_DESCRIPTOR *MemoryMap =
        AllocatePool(MemoryMapSize);

    if (MemoryMap == NULL)
    {
        Print(L"Failed to allocate memory map!\r\n");
        return EFI_OUT_OF_RESOURCES;
    }

    /*
     * Get the actual memory map.
     */
    Status = uefi_call_wrapper(
        BS->GetMemoryMap,
        5,
        &MemoryMapSize,
        MemoryMap,
        &MapKey,
        &DescriptorSize,
        &DescriptorVersion
    );

    if (EFI_ERROR(Status))
    {
        Print(
            L"GetMemoryMap failed: %r\r\n",
            Status
        );

        FreePool(MemoryMap);

        return Status;
    }

    Print(L"Memory map obtained successfully!\r\n");

    Print(
        L"Memory map size: 0x%lx\r\n",
        MemoryMapSize
    );

    Print(
        L"Descriptor size: 0x%lx\r\n",
        DescriptorSize
    );

    Print(
        L"Map key: 0x%lx\r\n",
        MapKey
    );

    /*
     * First attempt.
     */
    Print(L"Attempting ExitBootServices...\r\n");

    Status = uefi_call_wrapper(
        BS->ExitBootServices,
        2,
        ImageHandle,
        MapKey
    );

    if (!EFI_ERROR(Status))
    {
        /*
         * Boot Services are now gone.
         *
         * Do NOT FreePool(MemoryMap).
         */
        *OutMemoryMap = MemoryMap;
        *OutMemoryMapSize = MemoryMapSize;
        *OutDescriptorSize = DescriptorSize;

        return EFI_SUCCESS;
    }

    /*
     * The memory map changed between GetMemoryMap()
     * and ExitBootServices().
     */
    Print(
        L"First ExitBootServices failed: %r\r\n",
        Status
    );

    /*
     * Get a fresh map and fresh MapKey.
     */
    Status = uefi_call_wrapper(
        BS->GetMemoryMap,
        5,
        &MemoryMapSize,
        MemoryMap,
        &MapKey,
        &DescriptorSize,
        &DescriptorVersion
    );

    if (EFI_ERROR(Status))
    {
        Print(
            L"Second GetMemoryMap failed: %r\r\n",
            Status
        );

        FreePool(MemoryMap);

        return Status;
    }

    Print(L"Fresh memory map obtained.\r\n");

    Print(
        L"New map key: 0x%lx\r\n",
        MapKey
    );

    /*
     * Second attempt.
     */
    Print(L"Retrying ExitBootServices...\r\n");

    Status = uefi_call_wrapper(
        BS->ExitBootServices,
        2,
        ImageHandle,
        MapKey
    );

    if (EFI_ERROR(Status))
    {
        Print(
            L"Second ExitBootServices failed: %r\r\n",
            Status
        );

        FreePool(MemoryMap);

        return Status;
    }

    /*
     * Success.
     *
     * Do NOT free MemoryMap.
     */
    *OutMemoryMap = MemoryMap;
    *OutMemoryMapSize = MemoryMapSize;
    *OutDescriptorSize = DescriptorSize;

    return EFI_SUCCESS;
}


EFI_STATUS EFIAPI
efi_main(
    EFI_HANDLE ImageHandle,
    EFI_SYSTEM_TABLE *SystemTable
)
{
    EFI_STATUS Status;

    InitializeLib(
        ImageHandle,
        SystemTable
    );

    Print(L"\r\n");
    Print(L"===============================================\r\n");
    Print(L"        Snoopy AArch64 UEFI Loader             \r\n");
    Print(L"===============================================\r\n");


    /*
     * Locate filesystems.
     */
    EFI_HANDLE *Handles = NULL;
    UINTN HandleCount = 0;

    Status = LibLocateHandle(
        ByProtocol,
        &gEfiSimpleFileSystemProtocolGuid,
        NULL,
        &HandleCount,
        &Handles
    );

    if (EFI_ERROR(Status))
    {
        Print(
            L"Failed to locate EFI filesystems: %r\r\n",
            Status
        );

        return Status;
    }

    Print(
        L"Filesystem handles found: %u\r\n",
        HandleCount
    );


    /*
     * Find kernel.elf.
     */
    EFI_FILE_HANDLE Root = NULL;
    EFI_FILE_HANDLE KernelFile = NULL;

    for (UINTN i = 0; i < HandleCount; i++)
    {
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;

        Status = uefi_call_wrapper(
            BS->HandleProtocol,
            3,
            Handles[i],
            &gEfiSimpleFileSystemProtocolGuid,
            (void **)&FileSystem
        );

        if (EFI_ERROR(Status))
        {
            continue;
        }

        Status = uefi_call_wrapper(
            FileSystem->OpenVolume,
            2,
            FileSystem,
            &Root
        );

        if (EFI_ERROR(Status))
        {
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

        if (!EFI_ERROR(Status))
        {
            break;
        }

        Root = NULL;
    }

    if (KernelFile == NULL)
    {
        Print(L"kernel.elf not found!\r\n");

        FreePool(Handles);

        return EFI_NOT_FOUND;
    }

    Print(
        L"kernel.elf opened successfully!\r\n\r\n"
    );


    /*
     * Read ELF header.
     */
    Elf64_Ehdr ElfHeader;

    UINTN HeaderSize = sizeof(ElfHeader);

    Status = uefi_call_wrapper(
        KernelFile->Read,
        3,
        KernelFile,
        &HeaderSize,
        &ElfHeader
    );

    if (
        EFI_ERROR(Status) ||
        HeaderSize != sizeof(ElfHeader)
    )
    {
        Print(
            L"Failed to read ELF header: %r\r\n",
            Status
        );

        return EFI_LOAD_ERROR;
    }


    /*
     * Validate ELF.
     */
    Print(
        L"Validating kernel ELF...\r\n\r\n"
    );

    if (
        ElfHeader.e_ident[0] != ELF_MAGIC0 ||
        ElfHeader.e_ident[1] != ELF_MAGIC1 ||
        ElfHeader.e_ident[2] != ELF_MAGIC2 ||
        ElfHeader.e_ident[3] != ELF_MAGIC3
    )
    {
        Print(L"Invalid ELF magic!\r\n");
        return EFI_LOAD_ERROR;
    }

    if (ElfHeader.e_ident[4] != ELFCLASS64)
    {
        Print(L"Kernel is not ELF64!\r\n");
        return EFI_LOAD_ERROR;
    }

    if (ElfHeader.e_ident[5] != ELFDATA2LSB)
    {
        Print(L"Kernel is not little-endian!\r\n");
        return EFI_LOAD_ERROR;
    }

    if (ElfHeader.e_type != ET_EXEC)
    {
        Print(
            L"Kernel is not an executable ELF!\r\n"
        );

        return EFI_LOAD_ERROR;
    }

    if (ElfHeader.e_machine != EM_AARCH64)
    {
        Print(L"Kernel is not AArch64!\r\n");
        return EFI_LOAD_ERROR;
    }

    if (ElfHeader.e_phentsize != sizeof(Elf64_Phdr))
    {
        Print(
            L"Unexpected program header size!\r\n"
        );

        return EFI_LOAD_ERROR;
    }

    Print(
        L"Kernel ELF validated successfully!\r\n\r\n"
    );

    Print(
        L"Entry point:          0x%lx\r\n",
        ElfHeader.e_entry
    );

    Print(
        L"Program header offset: 0x%lx\r\n",
        ElfHeader.e_phoff
    );

    Print(
        L"Program header size:   0x%x\r\n",
        ElfHeader.e_phentsize
    );

    Print(
        L"Program header count:  %u\r\n",
        ElfHeader.e_phnum
    );


    /*
     * Read program headers.
     */
    UINTN ProgramHeaderSize =
        ElfHeader.e_phentsize *
        ElfHeader.e_phnum;

    Elf64_Phdr *ProgramHeaders =
        AllocatePool(ProgramHeaderSize);

    if (ProgramHeaders == NULL)
    {
        Print(
            L"Failed to allocate program headers!\r\n"
        );

        return EFI_OUT_OF_RESOURCES;
    }

    Status = uefi_call_wrapper(
        KernelFile->SetPosition,
        2,
        KernelFile,
        ElfHeader.e_phoff
    );

    if (EFI_ERROR(Status))
    {
        Print(
            L"Failed to seek to program headers: %r\r\n",
            Status
        );

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

    if (
        EFI_ERROR(Status) ||
        ReadSize != ProgramHeaderSize
    )
    {
        Print(
            L"Failed to read program headers: %r\r\n",
            Status
        );

        FreePool(ProgramHeaders);

        return EFI_LOAD_ERROR;
    }


    /*
     * Load PT_LOAD segments.
     */
    Print(L"\r\nProgram headers:\r\n");

    UINT64 KernelBase = 0;
    UINT64 KernelEnd = 0;

    for (UINTN i = 0; i < ElfHeader.e_phnum; i++)
    {
        Elf64_Phdr *Phdr =
            &ProgramHeaders[i];

        Print(
            L"\r\nProgram header [%u]\r\n"
            L"  Type:   0x%x\r\n"
            L"  Offset: 0x%lx\r\n"
            L"  VAddr:  0x%lx\r\n"
            L"  PAddr:  0x%lx\r\n"
            L"  FileSz: 0x%lx\r\n"
            L"  MemSz:  0x%lx\r\n"
            L"  Align:  0x%lx\r\n",
            i,
            Phdr->p_type,
            Phdr->p_offset,
            Phdr->p_vaddr,
            Phdr->p_paddr,
            Phdr->p_filesz,
            Phdr->p_memsz,
            Phdr->p_align
        );

        if (Phdr->p_type != PT_LOAD)
        {
            continue;
        }

        if (Phdr->p_filesz > Phdr->p_memsz)
        {
            Print(
                L"  -> Invalid PT_LOAD: filesz > memsz\r\n"
            );

            FreePool(ProgramHeaders);

            return EFI_LOAD_ERROR;
        }

        Print(L"  -> PT_LOAD found\r\n");

        EFI_PHYSICAL_ADDRESS SegmentAddress =
            Phdr->p_paddr;

        UINT64 SegmentSize =
            Phdr->p_memsz;

        Print(
            L"  -> Loading at physical address: 0x%lx\r\n",
            SegmentAddress
        );

        Print(
            L"  -> Memory size: 0x%lx bytes\r\n",
            SegmentSize
        );

        UINTN Pages =
            EFI_SIZE_TO_PAGES(SegmentSize);

        Print(
            L"  -> Pages required: %u\r\n",
            Pages
        );

        Status = uefi_call_wrapper(
            BS->AllocatePages,
            4,
            AllocateAddress,
            EfiLoaderData,
            Pages,
            &SegmentAddress
        );

        if (EFI_ERROR(Status))
        {
            Print(
                L"  -> AllocatePages failed: %r\r\n",
                Status
            );

            FreePool(ProgramHeaders);

            return Status;
        }

        Print(
            L"  -> Memory allocated at: 0x%lx\r\n",
            SegmentAddress
        );

        Status = uefi_call_wrapper(
            KernelFile->SetPosition,
            2,
            KernelFile,
            Phdr->p_offset
        );

        if (EFI_ERROR(Status))
        {
            Print(
                L"  -> Failed to seek to segment: %r\r\n",
                Status
            );

            FreePool(ProgramHeaders);

            return Status;
        }

        UINTN FileSize =
            Phdr->p_filesz;

        Status = uefi_call_wrapper(
            KernelFile->Read,
            3,
            KernelFile,
            &FileSize,
            (void *)(UINTN)SegmentAddress
        );

        if (
            EFI_ERROR(Status) ||
            FileSize != Phdr->p_filesz
        )
        {
            Print(
                L"  -> Failed to read segment: %r\r\n",
                Status
            );

            FreePool(ProgramHeaders);

            return EFI_LOAD_ERROR;
        }

        Print(
            L"  -> Copied 0x%lx bytes\r\n",
            Phdr->p_filesz
        );

        if (Phdr->p_memsz > Phdr->p_filesz)
        {
            UINT64 BssSize =
                Phdr->p_memsz -
                Phdr->p_filesz;

            void *BssAddress =
                (void *)(UINTN)(
                    SegmentAddress +
                    Phdr->p_filesz
                );

            SetMem(
                BssAddress,
                BssSize,
                0
            );

            Print(
                L"  -> Zeroed 0x%lx bytes of BSS\r\n",
                BssSize
            );
        }

        /*
         * Track complete kernel memory range.
         */
        if (KernelBase == 0)
        {
            KernelBase =
                Phdr->p_paddr;
        }

        UINT64 SegmentEnd =
            Phdr->p_paddr +
            Phdr->p_memsz;

        if (SegmentEnd > KernelEnd)
        {
            KernelEnd = SegmentEnd;
        }

        /*
         * Show first bytes.
         */
        UINT8 *Loaded =
            (UINT8 *)(UINTN)SegmentAddress;

        Print(
            L"  -> First bytes in memory: "
        );

        for (
            UINTN j = 0;
            j < 8 && j < Phdr->p_filesz;
            j++
        )
        {
            Print(
                L"%02x ",
                Loaded[j]
            );
        }

        Print(L"\r\n");

        Print(
            L"  -> Segment loaded successfully!\r\n"
        );
    }


    UINT64 KernelSize =
        KernelEnd - KernelBase;

    Print(
        L"\r\nSnoopy successfully loaded kernel.elf!\r\n"
    );

    Print(
        L"Kernel base: 0x%lx\r\n",
        KernelBase
    );

    Print(
        L"Kernel size: 0x%lx\r\n",
        KernelSize
    );

    Print(
        L"Kernel entry point: 0x%lx\r\n",
        ElfHeader.e_entry
    );


    /*
     * We don't need the program headers anymore.
     */
    FreePool(ProgramHeaders);


    /*
     * Close filesystem resources.
     */
    uefi_call_wrapper(
        KernelFile->Close,
        1,
        KernelFile
    );

    uefi_call_wrapper(
        Root->Close,
        1,
        Root
    );

    FreePool(Handles);


    /*
     * Allocate our boot information structure.
     *
     * This must happen BEFORE ExitBootServices().
     */
    BootInfo *BootInfoData =
        AllocatePool(sizeof(BootInfo));

    if (BootInfoData == NULL)
    {
        Print(
            L"Failed to allocate BootInfo!\r\n"
        );

        return EFI_OUT_OF_RESOURCES;
    }


    /*
     * Get memory map and exit UEFI.
     */
    Print(
        L"Exiting UEFI Boot Services...\r\n"
    );

    EFI_MEMORY_DESCRIPTOR *MemoryMap = NULL;
    UINTN MemoryMapSize = 0;
    UINTN DescriptorSize = 0;

    Status = ExitBootServicesWithRetry(
        ImageHandle,
        &MemoryMap,
        &MemoryMapSize,
        &DescriptorSize
    );

    if (EFI_ERROR(Status))
    {
        /*
         * Boot Services are still available because
         * ExitBootServices failed.
         */
        Print(
            L"ExitBootServices failed: %r\r\n",
            Status
        );

        FreePool(BootInfoData);

        return Status;
    }


    /*
     * Boot Services are now gone.
     *
     * We must NOT call any UEFI Boot Services anymore.
     */
    BootInfoData->memory_map =
        (uint64_t)(UINTN)MemoryMap;

    BootInfoData->memory_map_size =
        (uint64_t)MemoryMapSize;

    BootInfoData->memory_descriptor_size =
        (uint64_t)DescriptorSize;

    BootInfoData->kernel_base =
        KernelBase;

    BootInfoData->kernel_size =
        KernelSize;


    /*
     * Jump to kernel.
     *
     * BootInfoData is passed in x0 according to the
     * AArch64 calling convention.
     */
    KernelEntry Entry =
        (KernelEntry)(UINTN)ElfHeader.e_entry;

    Entry(BootInfoData);


    /*
     * kernel_main() should never return.
     */
    while (1)
    {
        __asm__ volatile("wfe");
    }

    return EFI_SUCCESS;
}
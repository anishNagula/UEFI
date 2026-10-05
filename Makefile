TARGET = build/BOOTAA64.EFI

GNU_EFI = third_party/gnu-efi
GNU_EFI_ARCH = $(GNU_EFI)/aarch64

CC = /opt/homebrew/bin/aarch64-elf-gcc
LD = /opt/homebrew/bin/aarch64-elf-ld
OBJCOPY = /opt/homebrew/bin/aarch64-elf-objcopy

EFI_INC = \
	-I$(GNU_EFI)/inc \
	-I$(GNU_EFI)/inc/aarch64 \
	-I$(GNU_EFI)/inc/protocol

EFI_LIB = $(GNU_EFI_ARCH)/lib
EFI_GNUEFI = $(GNU_EFI_ARCH)/gnuefi

EFI_CRT0 = $(EFI_GNUEFI)/crt0-efi-aarch64-local.o
EFI_LDS = $(GNU_EFI)/gnuefi/elf_aarch64_efi_local.lds

CFLAGS = \
	-fpic \
	-fshort-wchar \
	-fno-stack-protector \
	-fno-strict-aliasing \
	-ffreestanding \
	-fno-builtin \
	-fPIE \
	-Wall \
	-Wextra \
	-O2 \
	$(EFI_INC)

LDFLAGS = \
	-shared \
	-Bsymbolic \
	-L$(EFI_LIB) \
	-L$(EFI_GNUEFI) \
	$(EFI_CRT0) \
	-T $(EFI_LDS) \
	--defsym=EFI_SUBSYSTEM=0xa

all: $(TARGET) build/kernel.elf

build:
	mkdir -p build

build/main.o: src/main.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/snoopy.so: build/main.o
	$(LD) $(LDFLAGS) \
		-o $@ \
		build/main.o \
		-lefi \
		-lgnuefi \
		$(shell $(CC) -print-libgcc-file-name)

$(TARGET): build/snoopy.so
	$(OBJCOPY) \
		-j .text \
		-j .sdata \
		-j .data \
		-j .dynamic \
		-j .rodata \
		-j .rel \
		-j .rela \
		-j .rel.* \
		-j .rela.* \
		-j .rel* \
		-j .rela* \
		-j .areloc \
		-j .reloc \
		-O binary \
		$< $@

build/kernel.o: src/kernel.c src/bootinfo.h src/memory_map.h src/physical.h | build
	$(CC) \
		-ffreestanding \
		-fno-stack-protector \
		-fno-builtin \
		-O2 \
		-c $< -o $@

build/memory_map.o: src/memory_map.c src/memory_map.h src/bootinfo.h | build
	$(CC) \
		-ffreestanding \
		-fno-stack-protector \
		-fno-builtin \
		-O2 \
		-c $< -o $@

build/physical.o: src/physical.c src/physical.h src/memory_map.h src/bootinfo.h | build
	$(CC) \
		-ffreestanding \
		-fno-stack-protector \
		-fno-builtin \
		-O2 \
		-c $< -o $@

build/kernel_entry.o: src/kernel_entry.S | build
	$(CC) -c $< -o $@

build/vectors.o: src/vectors.S | build
	$(CC) -c $< -o $@

build/exception.o: src/exception.c | build
	$(CC) \
		-ffreestanding \
		-fno-stack-protector \
		-fno-builtin \
		-O2 \
		-c $< -o $@

build/kernel.elf: \
	build/kernel.o \
	build/memory_map.o \
	build/physical.o \
	build/kernel_entry.o \
	build/vectors.o \
	build/exception.o \
	src/kernel.ld

	/opt/homebrew/bin/aarch64-elf-ld \
		-T src/kernel.ld \
		-o $@ \
		build/kernel_entry.o \
		build/vectors.o \
		build/exception.o \
		build/memory_map.o \
		build/physical.o \
		build/kernel.o

install: all
	mkdir -p esp/EFI/BOOT
	cp build/BOOTAA64.EFI esp/EFI/BOOT/BOOTAA64.EFI
	cp build/kernel.elf esp/kernel.elf

clean:
	rm -rf build
	rm -f esp/EFI/BOOT/BOOTAA64.EFI
	rm -f esp/kernel.elf

.PHONY: all build install clean
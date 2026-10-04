TARGET = build/BOOTAA64.EFI

CC = /opt/homebrew/opt/llvm/bin/clang
LD = /opt/homebrew/bin/lld-link

CFLAGS = \
	-target aarch64-windows-msvc \
	-mgeneral-regs-only \
	-ffreestanding \
	-fno-stack-protector \
	-fno-builtin \
	-fshort-wchar \
	-Wall \
	-Wextra \
	-O2

LDFLAGS = \
	/machine:arm64 \
	/subsystem:efi_application \
	/entry:efi_main \
	/nodefaultlib \
	/out:$(TARGET)

all: $(TARGET)

build:
	mkdir -p build

build/main.o: src/main.c src/uefi.h | build
	$(CC) $(CFLAGS) -c src/main.c -o build/main.o

$(TARGET): build/main.o
	$(LD) $(LDFLAGS) build/main.o

install: $(TARGET)
	mkdir -p esp/EFI/BOOT
	cp $(TARGET) esp/EFI/BOOT/BOOTAA64.EFI

run: install
	./run.sh

clean:
	rm -rf build
	rm -f esp/EFI/BOOT/BOOTAA64.EFI
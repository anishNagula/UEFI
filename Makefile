TARGET = BOOTAA64.EFI

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

main.o: main.c uefi.h
	$(CC) $(CFLAGS) -c main.c -o main.o

$(TARGET): main.o
	$(LD) $(LDFLAGS) main.o

install: $(TARGET)
	mkdir -p esp/EFI/BOOT
	cp $(TARGET) esp/EFI/BOOT/BOOTAA64.EFI

clean:
	rm -f main.o $(TARGET)
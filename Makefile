bootloader:
	mkdir -p build/exec/
	nasm -fbin src/Bootloader/start.asm -o build/exec/Bootloader

kernel:
	mkdir -p build/temp/
	gcc -ffreestanding -c src/Kernel/main.c -o build/temp/kernel.o
	nasm -f elf64 src/Bootloader/KernelEntry/kernel_entry.asm -o build/temp/kernel_entry.o
	ld  -o build/exec/kernel.bin -Ttext 0x8000 build/temp/kernel_entry.o build/temp/kernel.o --oformat binary

os:
	mkdir -p build/os
	cat build/exec/Bootloader build/exec/kernel.bin > build/os/os-image

run:
	qemu-system-x86_64 build/os/os-image

iso:
	truncate build/exec/Bootloader -s 1200k
	mkisofs -o build/iso/OS -b build/exec/Bootloader ./build/exec/
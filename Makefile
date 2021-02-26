########################################################
#	PROJECT SETTINGS
########################################################
PROJECT_NAME 	= OS
OUTPUT_NAME 	= os-image
SRC_BASE 		= .
DEFINES 		= 

########################################################
#	DIRECTORIES
########################################################
BUILD_DIR = build/bin
TEMP_DIR = build/temp
KERNEL_SRC = src/Kernel
KLIB_SRC = src/lib
C_LIB_SRC = src/Libc/
C_POSIX_SRC = src/POSIX
INCLUDE_DIR = -I./include\
	-I./include/libc\
	-I./include/POSIX

########################################################
#	SOURCE FILES
########################################################
# Kernel
C_SRCS += $(shell find $(KERNEL_SRC) -name *.c)
ASM_SRCS += $(shell find $(KERNEL_SRC) -name *.asm)
GNU_ASM_SRCS += $(shell find $(KERNEL_SRC) -name *.S)
C_SRCS += $(shell find $(KLIB_SRC) -name *.c)

# libc 
C_SRCS += $(shell find $(C_LIB_SRC) -name *.c)

# pthread
C_SRCS += $(shell find $(C_POSIX_SRC) -name *.c)

########################################################
#	OBJECT FILES
########################################################

COBJECTS64		:= $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(C_SRCS))
ASMOBJECT64		:= $(patsubst %.asm, $(TEMP_DIR)/obj64/%.asm.o, $(ASM_SRCS))
GNU_ASMOBJECT64	:= $(patsubst %.S, $(TEMP_DIR)/obj64/%.S.o, $(GNU_ASM_SRCS))
ALL_OBJECTS64	:= $(sort $(COBJECTS64) $(ASMOBJECT64) $(GNU_ASMOBJECT64))

########################################################
#	COMPILER
########################################################
CC= ccache gcc

########################################################
#	COMPILER OPTIONS
########################################################
OPTIMIZATION = 

########################################################
#	COMPILER FLAGS
########################################################
CFLAGS = $(DEFINES) $(INCLUDE_DIR) -ffreestanding -nostdlib -fdiagnostics-color=always -Werror=return-type -Werror=implicit-int

########################################################
#	LINKER
########################################################
LD= ld

########################################################
#	LINKER OPTIONS
########################################################
LD_OPTIMIZATION = -flto 

########################################################
#	GENERATE OBJECT FILES
########################################################
OBJECTS = $(C_SRCS:.c=.o) $(ASM_SRCS:.asm=.asm.o) $(GNU_ASM_SRCS:.S=.S.o)

bootloader:
	mkdir -p $(BUILD_DIR)
	nasm -fbin src/Bootloader/start.asm -o build/bin/Bootloader

kernel: $(OBJECTS)
	mkdir -p $(BUILD_DIR)
	nasm -f elf64 src/Bootloader/KernelEntry/kernel_entry.asm -o build/temp/kernel_entry.o
	ld -o $(BUILD_DIR)/kernel.bin -T LinkerScript/Kernel.ld build/temp/kernel_entry.o $(ALL_OBJECTS64) -flto -z max-page-size=0x1000 --oformat binary

os:
	mkdir -p build/os
	cat build/bin/Bootloader $(BUILD_DIR)/kernel.bin > build/os/os-image.bin
	truncate build/os/os-image.bin -s 1200k
	dd if=build/os/os-image.bin of=files/filesys.dd bs=512 count=1 conv=notrunc
	dd if=build/os/os-image.bin of=files/filesys.dd bs=1 skip=512 seek=4014080 conv=notrunc
	cp files/filesys.dd build/os/os-image
	VBoxManage convertfromraw --format VDI build/os/os-image VBox/os-image.vdi

run:
	qemu-system-x86_64 build/os/os-image -monitor stdio -m 128 -no-reboot -no-shutdown

iso:
	cd ./build/os && mkdir -p files && cp os-image files/ && mkisofs -R -o balrog.iso -V BalrogOS -b os-image files/

run_debug:
	qemu-system-x86_64 -s -S build/os/os-image -monitor stdio

########################################################
#	GENERAL COMPILATION RULES
########################################################
%.o : %.c
	mkdir -p $(TEMP_DIR)/obj64/$(dir $<)
	$(CC) $(CFLAGS) $(OPTIMIZATION) -c $< -o $(TEMP_DIR)/obj64/$(<:.c=.o)

%.asm.o : %.asm
	mkdir -p $(TEMP_DIR)/obj64/$(dir $<)
	nasm -f elf64 $< -o $(TEMP_DIR)/obj64/$(<:.asm=.asm.o)

%.S.o : %.S
	mkdir -p $(TEMP_DIR)/obj64/$(dir $<)
	gcc -c $< -o $(TEMP_DIR)/obj64/$(<:.S=.S.o)

########################################################
#	CLEAN
########################################################
REMOVE = rm -Rf

clean:
	$(REMOVE) build/
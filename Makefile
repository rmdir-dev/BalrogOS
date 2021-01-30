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
C_LIBS_SRC = src/Libc/
INCLUDE_DIR = -I./include\
	-I./include/libc

########################################################
#	SOURCE FILES
########################################################
C_SRCS += $(wildcard $(KERNEL_SRC)/*.c)
C_SRCS += $(wildcard $(KERNEL_SRC)/*/*.c)
C_SRCS += $(wildcard $(KERNEL_SRC)/*/*/*.c)
C_SRCS += $(wildcard $(KERNEL_SRC)/*/*/*/*.c)
C_SRCS += $(wildcard $(KERNEL_SRC)/*/*/*/*/*.c)
C_SRCS += $(wildcard $(KERNEL_SRC)/*/*/*/*/*/*.c)
C_SRCS += $(wildcard $(KERNEL_SRC)/*/*/*/*/*/*/*.c)
C_SRCS += $(wildcard $(KERNEL_SRC)/*/*/*/*/*/*/*/*.c)
C_SRCS += $(wildcard $(KERNEL_SRC)/*/*/*/*/*/*/*/*/*.c)
C_SRCS += $(wildcard $(KERNEL_SRC)/*/*/*/*/*/*/*/*/*/*.c)

C_SRCS += $(wildcard $(C_LIBS_SRC)/*.c)
C_SRCS += $(wildcard $(C_LIBS_SRC)/*/*.c)
C_SRCS += $(wildcard $(C_LIBS_SRC)/*/*/*.c)
C_SRCS += $(wildcard $(C_LIBS_SRC)/*/*/*/*.c)
C_SRCS += $(wildcard $(C_LIBS_SRC)/*/*/*/*/*.c)
C_SRCS += $(wildcard $(C_LIBS_SRC)/*/*/*/*/*/*.c)
C_SRCS += $(wildcard $(C_LIBS_SRC)/*/*/*/*/*/*/*.c)
C_SRCS += $(wildcard $(C_LIBS_SRC)/*/*/*/*/*/*/*/*.c)
C_SRCS += $(wildcard $(C_LIBS_SRC)/*/*/*/*/*/*/*/*/*.c)
C_SRCS += $(wildcard $(C_LIBS_SRC)/*/*/*/*/*/*/*/*/*/*.c)

COBJECTS64		:= $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(C_SRCS))

ASM_SRCS += $(wildcard $(KERNEL_SRC)/*.asm)
ASM_SRCS += $(wildcard $(KERNEL_SRC)/*/*.asm)
ASM_SRCS += $(wildcard $(KERNEL_SRC)/*/*/*.asm)
ASM_SRCS += $(wildcard $(KERNEL_SRC)/*/*/*/*.asm)
ASM_SRCS += $(wildcard $(KERNEL_SRC)/*/*/*/*/*.asm)
ASM_SRCS += $(wildcard $(KERNEL_SRC)/*/*/*/*/*/*.asm)
ASM_SRCS += $(wildcard $(KERNEL_SRC)/*/*/*/*/*/*/*.asm)
ASM_SRCS += $(wildcard $(KERNEL_SRC)/*/*/*/*/*/*/*/*.asm)
ASM_SRCS += $(wildcard $(KERNEL_SRC)/*/*/*/*/*/*/*/*/*.asm)
ASM_SRCS += $(wildcard $(KERNEL_SRC)/*/*/*/*/*/*/*/*/*/*.asm)

ASMOBJECT64		:= $(patsubst %.asm, $(TEMP_DIR)/obj64/%.asm.o, $(ASM_SRCS))

ALL_OBJECTS64	:= $(sort $(COBJECTS64) $(ASMOBJECT64))

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
CFLAGS = $(DEFINES) $(INCLUDE_DIR) -ffreestanding -nostdlib

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
OBJECTS = $(C_SRCS:.c=.o) $(ASM_SRCS:.asm=.asm.o)

bootloader:
	mkdir -p $(BUILD_DIR)
	nasm -fbin src/Bootloader/start.asm -o build/bin/Bootloader

kernel: $(OBJECTS)
	mkdir -p $(BUILD_DIR)
	nasm -f elf64 src/Bootloader/KernelEntry/kernel_entry.asm -o build/temp/kernel_entry.o
	ld -o $(BUILD_DIR)/kernel.bin -T LinkerScript/Kernel.ld build/temp/kernel_entry.o $(ALL_OBJECTS64) -flto -z max-page-size=0x1000 --oformat binary

os:
	mkdir -p build/os
	cat build/bin/Bootloader $(BUILD_DIR)/kernel.bin > build/os/os-image
	truncate build/os/os-image -s 1200k

run:
	qemu-system-x86_64 build/os/os-image -monitor stdio -m 128

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

########################################################
#	CLEAN
########################################################
REMOVE = rm -Rf

clean:
	$(REMOVE) build/
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
OS_BUILD_DIR = build/os
BIN_BUILD_DIR = build/bin
SBIN_BUILD_DIR = build/sbin
TEMP_DIR = build/temp
KERNEL_SRC = src/Kernel
KLIB_SRC = src/klib
C_LIB_SRC = src/Libc/
C_POSIX_SRC = src/POSIX
LS_SRC = src/tool-kit/ls/
SH_SRC = src/tool-kit/sh/
BESH_SRC = src/tool-kit/besh/
ECHO_SRC = src/tool-kit/echo/
CAT_SRC = src/tool-kit/cat/
AUTH_SRC = src/tool-kit/auth/
CLEAR_SRC = src/tool-kit/clear/
SL_SRC = src/tool-kit/sl/
PWD_SRC = src/tool-kit/pwd/
HELLO_SRC = src/tool-kit/hello/
WHOAMI_SRC = src/tool-kit/whoami/
TLIB_SRC = src/tool-kit/tool-lib/
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
LIBC_SRCS += $(shell find $(C_LIB_SRC) -name *.c)

# pthread
PTHREADC_SRCS += $(shell find $(C_POSIX_SRC) -name *.c)

# tools
LS_SRCS = $(shell find $(LS_SRC) -name *.c)
SH_SRCS = $(shell find $(SH_SRC) -name *.c)
BESH_SRCS = $(shell find $(BESH_SRC) -name *.c)
HELLO_SRCS = $(shell find $(HELLO_SRC) -name *.c)
ECHO_SRCS = $(shell find $(ECHO_SRC) -name *.c)
CAT_SRCS = $(shell find $(CAT_SRC) -name *.c)
AUTH_SRCS = $(shell find $(AUTH_SRC) -name *.c)
CLEAR_SRCS = $(shell find $(CLEAR_SRC) -name *.c)
SL_SRCS = $(shell find $(SL_SRC) -name *.c)
WHOAMI_SRCS = $(shell find $(WHOAMI_SRC) -name *.c)
PWD_SRCS = $(shell find $(PWD_SRC) -name *.c)

# tool shared library
TLIB_SRCS = $(shell find $(TLIB_SRC) -name *.c)

########################################################
#	OBJECT FILES
########################################################

# Kernel
COBJECTS64		:= $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(C_SRCS))
ASMOBJECT64		:= $(patsubst %.asm, $(TEMP_DIR)/obj64/%.asm.o, $(ASM_SRCS))
GNU_ASMOBJECT64	:= $(patsubst %.S, $(TEMP_DIR)/obj64/%.S.o, $(GNU_ASM_SRCS))
ALL_KOBJECTS64	:= $(sort $(COBJECTS64) $(ASMOBJECT64) $(GNU_ASMOBJECT64))

# libc
LIBC_OBJECTS64 	:= $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(LIBC_SRCS))

# lib pthread
PSXC_OBJECTS64 	:= $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(PTHREADC_SRCS))

#tools
ALL_LS_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(LS_SRCS))
ALL_SH_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(SH_SRCS))
ALL_BESH_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(BESH_SRCS))
ALL_HELLO_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(HELLO_SRCS))
ALL_ECHO_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(ECHO_SRCS))
ALL_CAT_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(CAT_SRCS))
ALL_AUTH_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(AUTH_SRCS))
ALL_CLEAR_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(CLEAR_SRCS))
ALL_SL_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(SL_SRCS))
ALL_WHOAMI_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(WHOAMI_SRCS))
ALL_PWD_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(PWD_SRCS))

# tool shared library
ALL_TLIB_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(TLIB_SRCS))

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
CFLAGS = $(DEFINES) $(INCLUDE_DIR) -ffreestanding -nostdlib -fdiagnostics-color=always -Werror=return-type -Werror=implicit-int -Wno-address-of-packed-member

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
K_OBJECTS = $(C_SRCS:.c=.o) $(ASM_SRCS:.asm=.asm.o) $(GNU_ASM_SRCS:.S=.S.o)
LIBC_OBJECTS = $(LIBC_SRCS:.c=.o)
LIBPTH_OBJECTS = $(PTHREADC_SRCS:.c=.o)
TOOLS_OBJECT = $(LS_SRCS:.c=.o) $(SH_SRCS:.c=.o) $(HELLO_SRCS:.c=.o) $(ECHO_SRCS:.c=.o) $(CAT_SRCS:.c=.o) \
			$(AUTH_SRCS:.c=.o) $(CLEAR_SRCS:.c=.o) $(SL_SRCS:.c=.o) $(BESH_SRCS:.c=.o) $(PWD_SRCS:.c=.o) $(TLIB_SRCS:.c=.o) $(WHOAMI_SRCS:.c=.o)

bootloader:
	mkdir -p $(OS_BUILD_DIR)
	nasm -fbin src/Bootloader/start.asm -o $(OS_BUILD_DIR)/Bootloader

kernel: $(K_OBJECTS)
	mkdir -p $(OS_BUILD_DIR)
	nasm -f elf64 src/Bootloader/KernelEntry/kernel_entry.asm -o build/temp/kernel_entry.o
	ld -o $(OS_BUILD_DIR)/kernel.bin -T LinkerScript/Kernel.ld build/temp/kernel_entry.o $(ALL_KOBJECTS64) -flto -z max-page-size=0x1000 --oformat binary

os:
	mkdir -p build/os
	cat $(OS_BUILD_DIR)/Bootloader $(OS_BUILD_DIR)/kernel.bin > build/os/os-image.bin
	truncate build/os/os-image.bin -s 1200k
	dd if=build/os/os-image.bin of=files/filesys.dd bs=512 count=1 conv=notrunc
	dd if=build/os/os-image.bin of=files/filesys.dd bs=1 skip=512 seek=4014080 conv=notrunc
	cp files/filesys.dd build/os/os-image
#	mkdir VBox/ || true
#	rm VBox/os-image.vdi || true
#	VBoxManage convertfromraw --format VDI build/os/os-image VBox/os-image.vdi

tools: $(TOOLS_OBJECT) $(LIBC_OBJECTS) $(LIBPTH_OBJECTS)
	mkdir -p $(BIN_BUILD_DIR)
	mkdir -p $(SBIN_BUILD_DIR)
	ld -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/ls $(ALL_LS_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	ld -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/sh $(ALL_SH_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	ld -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/besh $(ALL_BESH_OBJECT64) $(PSXC_OBJECTS64) $(LIBC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	ld -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/hello $(ALL_HELLO_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	ld -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/echo $(ALL_ECHO_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	ld -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/cat $(ALL_CAT_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	ld -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(SBIN_BUILD_DIR)/auth $(ALL_AUTH_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	ld -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/clear $(ALL_CLEAR_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	ld -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/sl $(ALL_SL_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	ld -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/pwd $(ALL_PWD_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	ld -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/whoami $(ALL_WHOAMI_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	#$(PSXC_OBJECTS64)
	sudo mount -o loop files/filesys.dd files/root/
	sudo mkdir -p files/root/bin | true
	sudo mkdir -p files/root/sbin | true
	sudo cp -R build/bin/* files/root/bin/
	sudo cp -R build/sbin/* files/root/sbin/
	sudo chmod -R 777 files/root/bin/*
	sudo chmod -R 700 files/root/sbin/*
	sudo cp -R files/fs/* files/root/
	sudo chown -R root:root files/root
	sudo chmod 700 files/root/root
	sudo find files/root/root -type f -exec chmod u=rw,go= {} \;
	sudo find files/root/root -type d -exec chmod u=rwx,go= {} \;
	sudo chmod -R 664 files/root/etc/*
	sudo chmod 600 files/root/etc/shadow
	sudo chown root:root files/root/etc/shadow
	sudo chown -R 1000:1000 files/root/home/rmdir
	sudo umount files/filesys.dd

mount:
	sudo mount -o loop files/filesys.dd files/root/
	#sudo chown -R $(USER):$(USER) files/root/

mount_os_dd:
	sudo mount -o loop build/os/os-image files/root/
	#sudo chown -R $(USER):$(USER) files/root/


umount:
	#sudo chown -R root:root files/root/* | true
	sudo umount files/filesys.dd | true
	sudo umount files/os-image | true
	sudo umount files/root | true

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
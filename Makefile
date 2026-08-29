########################################################
#	PROJECT SETTINGS
########################################################
PROJECT_NAME 	= OS
OUTPUT_NAME 	= os-image
SRC_BASE 		= .
DEFINES 		= -DKDB_DEBUG -DKDB_DEFAULT_LVL=3 -DKDB_START_SEQ=0 -D__BALROG_VERSION__=\"0.0.1\"

########################################################
#	DIRECTORIES
########################################################
OS_BUILD_DIR = build/os
BIN_BUILD_DIR = build/bin
SBIN_BUILD_DIR = build/sbin
ROOT_BUILD_DIR = build/root/sbin
TEMP_DIR = build/temp
KERNEL_SRC = src/Kernel
KLIB_SRC = src/klib
C_LIB_SRC = src/Libc/
C_POSIX_SRC = src/POSIX
SHARED_SRC = src/Shared
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
DONUT_SRC = src/tool-kit/donut/
SETDEBUG_SRC = src/tool-kit/setdebug/
SLEEP_SRC = src/tool-kit/sleep/
TLIB_SRC = src/tool-kit/tool-lib/
INCLUDE_DIR = -I./include\
	-I./include/libc\
	-I./include/POSIX

########################################################
#	SOURCE FILES
########################################################
# shared between the kernel and the libc
SHARED_SRCS += $(shell find $(SHARED_SRC) -name *.c)

# Kernel
C_SRCS += $(shell find $(KERNEL_SRC) -name *.c)
ASM_SRCS += $(shell find $(KERNEL_SRC) -name *.asm)
GNU_ASM_SRCS += $(shell find $(KERNEL_SRC) -name *.S)
C_SRCS += $(shell find $(KLIB_SRC) -name *.c)
C_SRCS += $(SHARED_SRCS)

# libc 
LIBC_SRCS += $(shell find $(C_LIB_SRC) -name *.c)
LIBC_SRCS += $(SHARED_SRCS)

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
DONUT_SRCS = $(shell find $(DONUT_SRC) -name *.c)
SETDEBUG_SRCS = $(shell find $(SETDEBUG_SRC) -name *.c)
SLEEP_SRCS = $(shell find $(SLEEP_SRC) -name *.c)
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
ALL_DONUT_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(DONUT_SRCS))
ALL_SETDEBUG_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(SETDEBUG_SRCS))
ALL_SLEEP_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(SLEEP_SRCS))
ALL_PWD_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(PWD_SRCS))

# tool shared library
ALL_TLIB_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(TLIB_SRCS))

########################################################
#	TOOLBOX
########################################################
#	The kernel is built with its own x86_64-elf cross toolchain so the
#	host compiler and its headers never leak into it. Everything lands
#	in $(TOOLBOX_DIR), which is not versioned.
#
#	  make install_toolbox  build binutils, gcc and nasm into ./toolbox
#	  make check_toolbox    tell which binaries are missing
#	  make clean_toolbox    throw the whole toolbox away
#
#	qemu and mkisofs stay host tools, they are only needed by make run
#	and make iso, not to build the os.
TOOLBOX_DIR = $(CURDIR)/toolbox
TOOLBOX_BIN = $(TOOLBOX_DIR)/bin
TOOLBOX_SRC = $(TOOLBOX_DIR)/src
TOOLBOX_TARGET = x86_64-elf
TOOLBOX_JOBS = $(shell nproc)

BINUTILS_VERSION = 2.47
GCC_VERSION = 15.3.0
NASM_VERSION = 2.16.03

TOOLBOX_CC = $(TOOLBOX_BIN)/$(TOOLBOX_TARGET)-gcc
TOOLBOX_LD = $(TOOLBOX_BIN)/$(TOOLBOX_TARGET)-ld
TOOLBOX_OBJDUMP = $(TOOLBOX_BIN)/$(TOOLBOX_TARGET)-objdump
TOOLBOX_OBJCOPY = $(TOOLBOX_BIN)/$(TOOLBOX_TARGET)-objcopy
TOOLBOX_NASM = $(TOOLBOX_BIN)/nasm

########################################################
#	COMPILER
########################################################
CC = ccache $(TOOLBOX_CC)
NASM = $(TOOLBOX_NASM)
OBJDUMP = $(TOOLBOX_OBJDUMP)
OBJCOPY = $(TOOLBOX_OBJCOPY)

########################################################
#	COMPILER OPTIONS
########################################################
OPTIMIZATION =

#	The kernel is linked at 0xFFFFFF8000008000, far out of reach of the
#	32 bit relocations the small code model emits, so it is built with the
#	large model. The host gcc used to hide this by defaulting to PIE.
CODE_MODEL = -mcmodel=large

########################################################
#	DEBUG
########################################################
#	make debug rebuilds everything with dwarf symbols. The image
#	itself never changes, the symbols only live in kernel.elf which
#	gdb and CLion read on the side.
#
#	dwarf 4 is asked for on purpose, CLion and the qemu gdb stub
#	are happier with it than with the dwarf 5 gcc defaults to.
#
#	GDB stays a host tool, the toolbox does not build one.
GDB = gdb

ifeq ($(DEBUG),1)
DEBUG_FLAGS = -g -gdwarf-4 -fno-omit-frame-pointer
NASM_DEBUG_FLAGS = -g -F dwarf
endif

########################################################
#	COMPILER FLAGS
########################################################
# gcc 14 turned these three warnings into errors. the kernel relies on the
# old behaviour in a lot of places, so they are put back to warnings.
GCC14_FLAGS = -Wno-error=incompatible-pointer-types\
	-Wno-error=int-conversion\
	-Wno-error=implicit-function-declaration

CFLAGS = $(DEFINES) $(INCLUDE_DIR) $(GCC14_FLAGS) $(DEBUG_FLAGS) $(CODE_MODEL) -std=gnu17 -ffreestanding -nostdlib -fdiagnostics-color=always -Werror=return-type -Werror=implicit-int -Wno-address-of-packed-member

########################################################
#	LINKER
########################################################
LD = $(TOOLBOX_LD)

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
			$(AUTH_SRCS:.c=.o) $(CLEAR_SRCS:.c=.o) $(SL_SRCS:.c=.o) $(BESH_SRCS:.c=.o) $(PWD_SRCS:.c=.o) $(TLIB_SRCS:.c=.o) \
			$(WHOAMI_SRCS:.c=.o) $(DONUT_SRCS:.c=.o) $(SETDEBUG_SRCS:.c=.o) $(SLEEP_SRCS:.c=.o)

install_toolbox: $(TOOLBOX_LD) $(TOOLBOX_CC) $(TOOLBOX_NASM)
	@$(MAKE) --no-print-directory check_toolbox

#	every step is guarded so a failed build can be retried without
#	downloading and configuring everything again.
$(TOOLBOX_LD):
	mkdir -p $(TOOLBOX_SRC)/build-binutils
	[ -f $(TOOLBOX_SRC)/binutils-$(BINUTILS_VERSION).tar.xz ] || \
		(cd $(TOOLBOX_SRC) && curl -LO https://ftp.gnu.org/gnu/binutils/binutils-$(BINUTILS_VERSION).tar.xz)
	[ -d $(TOOLBOX_SRC)/binutils-$(BINUTILS_VERSION) ] || \
		(cd $(TOOLBOX_SRC) && tar xf binutils-$(BINUTILS_VERSION).tar.xz)
	[ -f $(TOOLBOX_SRC)/build-binutils/config.status ] || \
		(cd $(TOOLBOX_SRC)/build-binutils && ../binutils-$(BINUTILS_VERSION)/configure \
			--target=$(TOOLBOX_TARGET) --prefix=$(TOOLBOX_DIR) \
			--with-sysroot --disable-nls --disable-werror)
	$(MAKE) -C $(TOOLBOX_SRC)/build-binutils -j$(TOOLBOX_JOBS)
	$(MAKE) -C $(TOOLBOX_SRC)/build-binutils install

#	gcc needs the cross binutils to already be in the path, and only the
#	compiler itself plus libgcc are built, there is no libc to target.
$(TOOLBOX_CC): $(TOOLBOX_LD)
	mkdir -p $(TOOLBOX_SRC)/build-gcc
	[ -f $(TOOLBOX_SRC)/gcc-$(GCC_VERSION).tar.xz ] || \
		(cd $(TOOLBOX_SRC) && curl -LO https://ftp.gnu.org/gnu/gcc/gcc-$(GCC_VERSION)/gcc-$(GCC_VERSION).tar.xz)
	[ -d $(TOOLBOX_SRC)/gcc-$(GCC_VERSION) ] || \
		(cd $(TOOLBOX_SRC) && tar xf gcc-$(GCC_VERSION).tar.xz)
	[ -d $(TOOLBOX_SRC)/gcc-$(GCC_VERSION)/gmp ] || \
		(cd $(TOOLBOX_SRC)/gcc-$(GCC_VERSION) && ./contrib/download_prerequisites)
	[ -f $(TOOLBOX_SRC)/build-gcc/config.status ] || \
		(cd $(TOOLBOX_SRC)/build-gcc && PATH=$(TOOLBOX_BIN):$$PATH ../gcc-$(GCC_VERSION)/configure \
			--target=$(TOOLBOX_TARGET) --prefix=$(TOOLBOX_DIR) \
			--disable-nls --enable-languages=c --without-headers)
	PATH=$(TOOLBOX_BIN):$$PATH $(MAKE) -C $(TOOLBOX_SRC)/build-gcc all-gcc -j$(TOOLBOX_JOBS)
	PATH=$(TOOLBOX_BIN):$$PATH $(MAKE) -C $(TOOLBOX_SRC)/build-gcc all-target-libgcc -j$(TOOLBOX_JOBS)
	$(MAKE) -C $(TOOLBOX_SRC)/build-gcc install-gcc
	$(MAKE) -C $(TOOLBOX_SRC)/build-gcc install-target-libgcc

$(TOOLBOX_NASM):
	mkdir -p $(TOOLBOX_SRC)
	[ -f $(TOOLBOX_SRC)/nasm-$(NASM_VERSION).tar.xz ] || \
		(cd $(TOOLBOX_SRC) && curl -LO https://www.nasm.us/pub/nasm/releasebuilds/$(NASM_VERSION)/nasm-$(NASM_VERSION).tar.xz)
	[ -d $(TOOLBOX_SRC)/nasm-$(NASM_VERSION) ] || \
		(cd $(TOOLBOX_SRC) && tar xf nasm-$(NASM_VERSION).tar.xz)
	[ -f $(TOOLBOX_SRC)/nasm-$(NASM_VERSION)/config.status ] || \
		(cd $(TOOLBOX_SRC)/nasm-$(NASM_VERSION) && ./configure --prefix=$(TOOLBOX_DIR))
	$(MAKE) -C $(TOOLBOX_SRC)/nasm-$(NASM_VERSION) -j$(TOOLBOX_JOBS)
	$(MAKE) -C $(TOOLBOX_SRC)/nasm-$(NASM_VERSION) install

check_toolbox:
	@missing=0; \
	for tool in $(TOOLBOX_CC) $(TOOLBOX_LD) $(TOOLBOX_OBJDUMP) $(TOOLBOX_NASM); do \
		if [ -x $$tool ]; then \
			echo "[  OK  ] $$tool"; \
		else \
			echo "[FAILED] $$tool"; \
			missing=1; \
		fi; \
	done; \
	for tool in qemu-system-x86_64 mkisofs; do \
		if command -v $$tool > /dev/null; then \
			echo "[  OK  ] $$tool (host)"; \
		else \
			echo "[ INFO ] $$tool (host) is missing, only needed by make run and make iso"; \
		fi; \
	done; \
	if [ $$missing -ne 0 ]; then \
		echo "toolbox is incomplete, run make install_toolbox"; \
		exit 1; \
	fi; \
	echo "toolbox is complete."

#	the sources are only needed while building, the binaries stay.
clean_toolbox_src:
	$(REMOVE) $(TOOLBOX_SRC)

clean_toolbox:
	$(REMOVE) $(TOOLBOX_DIR)

bootloader:
	mkdir -p $(OS_BUILD_DIR)
	$(NASM) -fbin src/Bootloader/start.asm -o $(OS_BUILD_DIR)/Bootloader

kernel: $(K_OBJECTS)
	mkdir -p $(OS_BUILD_DIR)
	$(NASM) -f elf64 $(NASM_DEBUG_FLAGS) src/Bootloader/KernelEntry/kernel_entry.asm -o build/temp/kernel_entry.o
#	linked as an elf so the symbols survive, the raw image the
#	bootloader loads is carved out of it right after.
	$(LD) -o $(OS_BUILD_DIR)/kernel.elf -T LinkerScript/Kernel.ld build/temp/kernel_entry.o $(ALL_KOBJECTS64) -flto -z max-page-size=0x1000
	$(OBJCOPY) -O binary $(OS_BUILD_DIR)/kernel.elf $(OS_BUILD_DIR)/kernel.bin

h_readble_kernel_asm: $(K_OBJECTS)
	mkdir -p $(OS_BUILD_DIR)
	$(NASM) -f elf64 src/Bootloader/KernelEntry/kernel_entry.asm -o build/temp/kernel_entry.o
	$(LD) -S -o $(OS_BUILD_DIR)/kernel.asm -T LinkerScript/Kernel.ld build/temp/kernel_entry.o $(ALL_KOBJECTS64) -flto -z max-page-size=0x1000
	$(OBJDUMP) -S $(OS_BUILD_DIR)/kernel.asm > $(OS_BUILD_DIR)/kernel.asm.txt
	rm $(OS_BUILD_DIR)/kernel.asm

os:
	mkdir -p build/os
	cat $(OS_BUILD_DIR)/Bootloader $(OS_BUILD_DIR)/kernel.bin > build/os/os-image.bin
	truncate build/os/os-image.bin -s 1200k
	dd if=build/os/os-image.bin of=files/filesys.dd bs=512 count=1 conv=notrunc
	dd if=build/os/os-image.bin of=files/filesys.dd bs=1 skip=512 seek=4014080 conv=notrunc
	cp files/filesys.dd build/os/os-image
#	the vdi is rebuilt from scratch every time, VirtualBox will not pick
#	up a raw image that changed under an image it already converted.
	mkdir -p VBox/
	rm -f VBox/os-image.vdi
	VBoxManage convertfromraw --format VDI build/os/os-image VBox/os-image.vdi

tools: $(TOOLS_OBJECT) $(LIBC_OBJECTS) $(LIBPTH_OBJECTS)
	mkdir -p $(BIN_BUILD_DIR)
	mkdir -p $(SBIN_BUILD_DIR)
	mkdir -p $(ROOT_BUILD_DIR)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/ls $(ALL_LS_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/sh $(ALL_SH_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/besh $(ALL_BESH_OBJECT64) $(PSXC_OBJECTS64) $(LIBC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/hello $(ALL_HELLO_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/echo $(ALL_ECHO_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/cat $(ALL_CAT_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(SBIN_BUILD_DIR)/auth $(ALL_AUTH_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/clear $(ALL_CLEAR_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/sl $(ALL_SL_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/pwd $(ALL_PWD_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/whoami $(ALL_WHOAMI_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/donut $(ALL_DONUT_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/sleep $(ALL_SLEEP_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(ROOT_BUILD_DIR)/setdebug $(ALL_SETDEBUG_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	#$(PSXC_OBJECTS64)
	sudo mount -o loop files/filesys.dd files/root/
	sudo mkdir -p files/root/bin | true
	sudo mkdir -p files/root/sbin | true
	sudo mkdir -p files/root/root/sbin | true
	sudo cp -R build/bin/* files/root/bin/
	sudo cp -R build/sbin/* files/root/sbin/
	sudo cp -R build/root/sbin/* files/root/root/sbin/
	sudo chmod -R 777 files/root/bin/*
	sudo chmod -R 750 files/root/sbin/*
	sudo chmod -R 700 files/root/root/sbin
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
	#qemu-system-x86_64 build/os/os-image -monitor stdio -m 128 -no-reboot -no-shutdown
	qemu-system-x86_64 -monitor stdio -m 128 -no-reboot -no-shutdown \
		-drive id=disk,file=build/os/os-image,if=none \
		-device ahci,id=ahci \
		-device ide-hd,drive=disk,bus=ahci.0

iso:
	cd ./build/os && mkdir -p files && cp os-image files/ && mkisofs -R -o balrog.iso -V BalrogOS -b Booloader files/

#	full rebuild with symbols. the objects are thrown away first,
#	they were compiled without -g and make would keep them.
debug:
	$(MAKE) clean
	$(MAKE) DEBUG=1 bootloader
	$(MAKE) DEBUG=1 kernel
	$(MAKE) DEBUG=1 os
	@echo "[  OK  ] symbols are in $(OS_BUILD_DIR)/kernel.elf, now run make run_debug"

run_debug:
	qemu-system-x86_64 -s -S build/os/os-image -monitor stdio -m 128 -no-reboot -no-shutdown

#	attach to the qemu left waiting by make run_debug.
#	CLion does the same thing through a Remote Debug configuration.
gdb:
	$(GDB) $(OS_BUILD_DIR)/kernel.elf \
		-ex "target remote localhost:1234" \
		-ex "break kernel_main" \
		-ex "continue"

########################################################
#	GENERAL COMPILATION RULES
########################################################
%.o : %.c
	mkdir -p $(TEMP_DIR)/obj64/$(dir $<)
	$(CC) $(CFLAGS) $(OPTIMIZATION) -c $< -o $(TEMP_DIR)/obj64/$(<:.c=.o)

%.asm.o : %.asm
	mkdir -p $(TEMP_DIR)/obj64/$(dir $<)
	$(NASM) -f elf64 $< -o $(TEMP_DIR)/obj64/$(<:.asm=.asm.o)

%.S.o : %.S
	mkdir -p $(TEMP_DIR)/obj64/$(dir $<)
	$(CC) -c $< -o $(TEMP_DIR)/obj64/$(<:.S=.S.o)

########################################################
#	CLEAN
########################################################
REMOVE = rm -Rf

clean:
	$(REMOVE) build/
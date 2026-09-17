########################################################
#	PROJECT SETTINGS
########################################################
PROJECT_NAME 	= OS
OUTPUT_NAME 	= os-image
SRC_BASE 		= .
DEFINES 		= -DKDB_DEBUG -DKDB_DEFAULT_LVL=3 -DKDB_START_SEQ=0 -D__BALROG_VERSION__=\"0.0.1\"

########################################################
#	BOOTLOADER LAYOUT
########################################################
#	keep in sync with src/bootloader/common/layout.inc, make cannot include a
#	nasm file so the two carry the same numbers. layout.inc is the one
#	that explains where they come from.
#
#	   lba 0        the mbr : stage 1 and the partition table
#	   lba 1-2047   the mbr gap, where grub puts core.img and we put stage 2
#	   lba 2048     partition 1, the kernel then the ramfs, raw
STAGE2_LBA 		= 1
PART1_LBA 		= 2048
KERNEL_LBA 		= 2048
KERNEL_SECTORS 	= 768
RAMFS_LBA 		= 2816
PART1_SECTORS 	= 17152
OS_IMAGE 		= build/os/os-image
ISO_ROOT 		= build/isoroot

########################################################
#	DIRECTORIES
########################################################
OS_BUILD_DIR = build/os
BIN_BUILD_DIR = build/bin
SBIN_BUILD_DIR = build/sbin
ROOT_BUILD_DIR = build/root/sbin
TEMP_DIR = build/temp
KERNEL_SRC = src/kernel
KLIB_SRC = src/klib
C_LIB_SRC = src/libc/
C_POSIX_SRC = src/posix
SHARED_SRC = src/shared
LS_SRC = src/tool_kit/ls/
SH_SRC = src/tool_kit/sh/
BESH_SRC = src/tool_kit/besh/
ECHO_SRC = src/tool_kit/echo/
CAT_SRC = src/tool_kit/cat/
AUTH_SRC = src/tool_kit/auth/
CLEAR_SRC = src/tool_kit/clear/
SL_SRC = src/tool_kit/sl/
PWD_SRC = src/tool_kit/pwd/
TOUCH_SRC = src/tool_kit/touch/
MKDIR_SRC = src/tool_kit/mkdir/
RM_SRC = src/tool_kit/rm/
RMDIR_SRC = src/tool_kit/rmdir/
HELLO_SRC = src/tool_kit/hello/
WHOAMI_SRC = src/tool_kit/whoami/
DONUT_SRC = src/tool_kit/donut/
SETDEBUG_SRC = src/tool_kit/setdebug/
SLEEP_SRC = src/tool_kit/sleep/
SHUTDOWN_SRC = src/tool_kit/shutdown/
TLIB_SRC = src/tool_kit/tool_lib/

#	lai is a git submodule, an AML interpreter. we need it to evaluate the
#	_PTS method the firmware wants before a shutdown. _PTS is code and not
#	data, so we cannot read it out of the DSDT the way we read _S5_.
#	run  git submodule update --init  if the directory is empty.
LAI_SRC = lai

#	the uefi loader. it is a second way into the same kernel | the bios chain
#	in start.asm still exists and still works, but a machine built after 2020
#	has no CSM anymore and will never read our mbr.
UEFI_SRC = src/bootloader/uefi
ESP_DIR = build/esp

OS_LOG_DIR = logs/
INCLUDE_DIR = -I./include\
	-I./include/libc\
	-I./include/posix\
	-I./$(LAI_SRC)/include\
	-I./$(TEMP_DIR)

########################################################
#	SOURCE FILES
########################################################
# shared between the kernel and the libc
SHARED_SRCS += $(shell find $(SHARED_SRC) -name *.c)

# Kernel
C_SRCS += $(shell find $(KERNEL_SRC) -name *.c)

#	core is the interpreter itself, helpers holds lai_enter_sleep, and
#	drivers holds the embedded controller that _PTS talks to on a laptop.
LAI_SRCS += $(shell find $(LAI_SRC)/core $(LAI_SRC)/helpers $(LAI_SRC)/drivers -name *.c 2>/dev/null)
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
SHUTDOWN_SRCS = $(shell find $(SHUTDOWN_SRC) -name *.c)
PWD_SRCS = $(shell find $(PWD_SRC) -name *.c)
TOUCH_SRCS = $(shell find $(TOUCH_SRC) -name *.c)
MKDIR_SRCS = $(shell find $(MKDIR_SRC) -name *.c)
RM_SRCS = $(shell find $(RM_SRC) -name *.c)
RMDIR_SRCS = $(shell find $(RMDIR_SRC) -name *.c)

# tool shared library
TLIB_SRCS = $(shell find $(TLIB_SRC) -name *.c)

########################################################
#	OBJECT FILES
########################################################

# Kernel
COBJECTS64		:= $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(C_SRCS))
ASMOBJECT64		:= $(patsubst %.asm, $(TEMP_DIR)/obj64/%.asm.o, $(ASM_SRCS))
GNU_ASMOBJECT64	:= $(patsubst %.S, $(TEMP_DIR)/obj64/%.S.o, $(GNU_ASM_SRCS))
LAI_OBJECTS64	:= $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(LAI_SRCS))

ALL_KOBJECTS64	:= $(sort $(COBJECTS64) $(ASMOBJECT64) $(GNU_ASMOBJECT64) $(LAI_OBJECTS64))

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
ALL_SHUTDOWN_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(SHUTDOWN_SRCS))
ALL_PWD_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(PWD_SRCS))
ALL_TOUCH_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(TOUCH_SRCS))
ALL_MKDIR_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(MKDIR_SRCS))
ALL_RM_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(RM_SRCS))
ALL_RMDIR_OBJECT64 := $(patsubst %.c, $(TEMP_DIR)/obj64/%.o, $(RMDIR_SRCS))

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

#	e2fsprogs stays a host tool too. debian installs it in /sbin, which is
#	not in a normal user PATH, so the full path is spelled out here and the
#	targets do not depend on how the shell is set up.
#	making a filesystem inside a regular file needs no privilege, only the
#	`mount -o loop` further down does.
#	OVMF, the free uefi firmware, for run_uefi. debian puts it there.
OVMF_CODE = /usr/share/OVMF/OVMF_CODE_4M.fd
OVMF_VARS = /usr/share/OVMF/OVMF_VARS_4M.fd

NM = $(TOOLBOX_BIN)/$(TOOLBOX_TARGET)-nm

#	OVMF, the free uefi firmware, for run_uefi. debian puts it there.
OVMF_CODE = /usr/share/OVMF/OVMF_CODE_4M.fd
OVMF_VARS = /usr/share/OVMF/OVMF_VARS_4M.fd

NM = $(TOOLBOX_BIN)/$(TOOLBOX_TARGET)-nm

MKE2FS = /sbin/mke2fs
DUMPE2FS = /sbin/dumpe2fs
DEBUGFS_BIN = /sbin/debugfs

SFDISK = /sbin/sfdisk
MKFS_VFAT = /sbin/mkfs.vfat
MCOPY = /usr/bin/mcopy

#	debugfs pages its own output whenever stdout is a terminal, and a make
#	running in a terminal is one : the build stops on a (END) prompt and
#	waits for a keypress. PAGER=cat turns the pager off.
DEBUGFS = PAGER=cat $(DEBUGFS_BIN)

ifeq ($(DEBUG),1)
DEBUG_FLAGS = -g -gdwarf-4 -fno-omit-frame-pointer
NASM_DEBUG_FLAGS = -g -F dwarf
endif

########################################################
#	RAMFS
########################################################
RAMFS_IMG = files/ramfs.img
RAMFS_SIZE_MiB = 8
RAMFS_MNT = files/ramfs_root
RAMFS_STAGE = build/ramfs_root

#	the hierarchy, as a make list and not a shell brace expansion : make runs
#	its recipes with /bin/sh, dash on debian, which does not do brace
#	expansion. `mkdir -p x/{a,b}` there makes a directory named "{a,b}".
RAMFS_DIRS = bin sbin etc root home boot tmp mnt dev proc sys lib var

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
#	LAI_SRCS is here and not only in ALL_KOBJECTS64. the kernel target
#	depends on this list, so this is what gets compiled. ALL_KOBJECTS64
#	only names the objects we give to the linker.
K_OBJECTS = $(C_SRCS:.c=.o) $(ASM_SRCS:.asm=.asm.o) $(GNU_ASM_SRCS:.S=.S.o) $(LAI_SRCS:.c=.o)
LIBC_OBJECTS = $(LIBC_SRCS:.c=.o)
LIBPTH_OBJECTS = $(PTHREADC_SRCS:.c=.o)
TOOLS_OBJECT = $(LS_SRCS:.c=.o) $(SH_SRCS:.c=.o) $(HELLO_SRCS:.c=.o) $(ECHO_SRCS:.c=.o) $(CAT_SRCS:.c=.o) \
			$(AUTH_SRCS:.c=.o) $(CLEAR_SRCS:.c=.o) $(SL_SRCS:.c=.o) $(BESH_SRCS:.c=.o) $(PWD_SRCS:.c=.o) $(TLIB_SRCS:.c=.o) \
			$(WHOAMI_SRCS:.c=.o) $(DONUT_SRCS:.c=.o) $(SETDEBUG_SRCS:.c=.o) $(SLEEP_SRCS:.c=.o) $(SHUTDOWN_SRCS:.c=.o) \
			$(TOUCH_SRCS:.c=.o) $(MKDIR_SRCS:.c=.o) $(RM_SRCS:.c=.o) $(RMDIR_SRCS:.c=.o)

########################################################
#	PHONY TARGETS
########################################################
#	none of the rules below creates a file named after itself, they all
#	build through a side effect. without this make finds the build/
#	directory and answers "'build' is up to date" instead of running the rule.
.PHONY: install_toolbox check_toolbox clean_toolbox_src clean_toolbox \
		bootloader kernel h_readble_kernel_asm os tools ramfs \
		sync_rootfs mount mount_os_dd umount uefi esp iso \
		run run_uefi run_debug run_debug_efi run_uefi_q35 \
		release release_uefi debug log gdb clean \
		build build_debug build_all build_all_debug

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
			--with-sysroot --disable-nls --disable-werror \
			--enable-targets=x86_64-pep)
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
	for tool in $(MKE2FS) $(DUMPE2FS) $(DEBUGFS_BIN); do \
		if [ -x $$tool ]; then \
			echo "[  OK  ] $$tool (host)"; \
		else \
			echo "[ INFO ] $$tool (host) is missing, only needed by make ramfs"; \
		fi; \
	done; \
	for tool in $(SFDISK) $(MKFS_VFAT) $(MCOPY); do \
		if [ -x $$tool ]; then \
			echo "[  OK  ] $$tool (host)"; \
		else \
			echo "[ INFO ] $$tool (host) is missing, only needed by make release_uefi"; \
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
	$(NASM) -fbin src/bootloader/bios/start.asm -o $(OS_BUILD_DIR)/Bootloader
#	stage 2 is padded to STAGE2_SECTORS * 512 by the times at the end of
#	stage2.asm, so KERNEL_LBA never moves when the code in it grows.
	$(NASM) -fbin src/bootloader/bios/stage2.asm -o $(OS_BUILD_DIR)/Stage2

kernel: $(K_OBJECTS)
	@if [ -z "$(LAI_SRCS)" ]; then \
		echo "[FAILED] $(LAI_SRC) is empty, run : git submodule update --init"; \
		exit 1; \
	fi
	mkdir -p $(OS_BUILD_DIR)
	mkdir -p $(OS_LOG_DIR)
	$(NASM) -f elf64 $(NASM_DEBUG_FLAGS) src/bootloader/common/kernel_entry/kernel_entry.asm -o build/temp/kernel_entry.o
#	linked as an elf so the symbols survive, the raw image the
#	bootloader loads is carved out of it right after.
	$(LD) -o $(OS_BUILD_DIR)/kernel.elf -T LinkerScript/Kernel.ld build/temp/kernel_entry.o $(ALL_KOBJECTS64) -flto -z max-page-size=0x1000
	$(OBJCOPY) -O binary $(OS_BUILD_DIR)/kernel.elf $(OS_BUILD_DIR)/kernel.bin

h_readble_kernel_asm: $(K_OBJECTS)
	mkdir -p $(OS_BUILD_DIR)
	$(NASM) -f elf64 src/bootloader/common/kernel_entry/kernel_entry.asm -o build/temp/kernel_entry.o
	$(LD) -S -o $(OS_BUILD_DIR)/kernel.asm -T LinkerScript/Kernel.ld build/temp/kernel_entry.o $(ALL_KOBJECTS64) -flto -z max-page-size=0x1000
	$(OBJDUMP) -S $(OS_BUILD_DIR)/kernel.asm > $(OS_BUILD_DIR)/kernel.asm.txt
	rm $(OS_BUILD_DIR)/kernel.asm

os:
	mkdir -p build/os
#	stage 2 reads exactly KERNEL_SECTORS sectors and no more, so a kernel
#	that outgrows them boots half loaded and fails somewhere else entirely.
	@KSIZE=$$(stat -c%s $(OS_BUILD_DIR)/kernel.bin); \
	MAX=$$(( $(KERNEL_SECTORS) * 512 )); \
	if [ $$KSIZE -gt $$MAX ]; then \
		echo "[FAILED] kernel.bin is $$KSIZE bytes, KERNEL_SECTORS covers $$MAX"; \
		echo "         raise KERNEL_SECTORS in src/bootloader/common/layout.inc and in this file"; \
		exit 1; \
	fi
	@if [ ! -f $(RAMFS_IMG) ]; then \
		echo "[FAILED] $(RAMFS_IMG) is missing, run make ramfs first"; \
		exit 1; \
	fi
#	the image is built from nothing every time, each piece written at the lba
#	layout.inc gives it : an mbr, a gap holding stage 2, then partition 1.
	dd if=/dev/zero of=$(OS_IMAGE) bs=512 count=$$(( $(PART1_LBA) + $(PART1_SECTORS) )) status=none
	dd if=$(OS_BUILD_DIR)/Bootloader of=$(OS_IMAGE) bs=512 seek=0 conv=notrunc status=none
	dd if=$(OS_BUILD_DIR)/Stage2 of=$(OS_IMAGE) bs=512 seek=$(STAGE2_LBA) conv=notrunc status=none
	dd if=$(OS_BUILD_DIR)/kernel.bin of=$(OS_IMAGE) bs=512 seek=$(KERNEL_LBA) conv=notrunc status=none
	dd if=$(RAMFS_IMG) of=$(OS_IMAGE) bs=512 seek=$(RAMFS_LBA) conv=notrunc status=none
	@echo "[  OK  ] mbr at 0, stage 2 at $(STAGE2_LBA), kernel at $(KERNEL_LBA), ramfs at $(RAMFS_LBA)"
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
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/touch $(ALL_TOUCH_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/mkdir $(ALL_MKDIR_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/rm $(ALL_RM_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/rmdir $(ALL_RMDIR_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/whoami $(ALL_WHOAMI_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/donut $(ALL_DONUT_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/sleep $(ALL_SLEEP_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(BIN_BUILD_DIR)/shutdown $(ALL_SHUTDOWN_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	$(LD) -m elf_x86_64 -N -e _start -Ttext 0x4000 -z max-page-size=0x1000 -o $(ROOT_BUILD_DIR)/setdebug $(ALL_SETDEBUG_OBJECT64) $(LIBC_OBJECTS64) $(PSXC_OBJECTS64) $(ALL_TLIB_OBJECT64)
	#$(PSXC_OBJECTS64)
	sudo mount -o loop files/filesys.dd files/root/
	sudo mkdir -p $(addprefix files/root/,$(RAMFS_DIRS))
	sudo mkdir -p files/root/root/sbin
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

ramfs:
#	the image is filled with mke2fs -d, from a staging directory, and never
#	mounted, so this target needs no sudo.
	$(REMOVE) $(RAMFS_STAGE)
	mkdir -p $(RAMFS_STAGE)

#	the filesystem hierarchy. /mnt for a future installer, /dev /proc /sys
#	for the drivers that will want them, /lib for shared objects.
	mkdir -p $(addprefix $(RAMFS_STAGE)/,$(RAMFS_DIRS))

#	the binaries the toolkit just built
	cp $(BIN_BUILD_DIR)/*  $(RAMFS_STAGE)/bin/
	cp $(SBIN_BUILD_DIR)/* $(RAMFS_STAGE)/sbin/

#	the versioned configuration and home directories. files/fs is the source,
#	files/root is only the mount point of the `mount` target.
#	the dot in files/fs/root/. matters : it copies .spade, which * would skip.
	cp -r files/fs/etc/*  $(RAMFS_STAGE)/etc/
	cp -r files/fs/root/. $(RAMFS_STAGE)/root/
	cp -r files/fs/home/* $(RAMFS_STAGE)/home/

#	/tmp is world writable, the rest is not
	chmod 1777 $(RAMFS_STAGE)/tmp

	dd if=/dev/zero of=$(RAMFS_IMG) bs=1M count=$(RAMFS_SIZE_MiB) status=none
	$(MKE2FS) -b 4096 -I 128 -m 0 -F -d $(RAMFS_STAGE) $(RAMFS_IMG)
	$(DUMPE2FS) -h $(RAMFS_IMG)
	$(DEBUGFS) -R "ls -l /" $(RAMFS_IMG)
	@echo "[  OK  ] ramfs image built, $(RAMFS_SIZE_MiB) MiB"

sync_rootfs:
	sudo mount -o loop files/filesys.dd files/root
	sudo mkdir -p $(addprefix files/root/,$(RAMFS_DIRS))
	sudo cp -r files/fs/etc/*  files/root/etc/
	sudo cp -r files/fs/root/. files/root/root/
	sudo cp -r files/fs/home/* files/root/home/
	sudo chmod 1777 files/root/tmp
	sudo umount files/root
	$(DUMPE2FS) -h files/filesys.dd
	$(DEBUGFS) -R "ls -l /" files/filesys.dd

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
#	COM1 goes to a file : stdio is already taken by the monitor. the log is
#	complete, greppable, and diffable between two runs, which the VGA text
#	console is not once it starts scrolling.
	mv $(OS_LOG_DIR)/kernel.log $(OS_LOG_DIR)/kernel.log.bak | true
	qemu-system-x86_64 -monitor stdio -m 4096 -no-reboot -no-shutdown \
		-drive id=disk,file=build/os/os-image,format=raw,if=none \
		-device ahci,id=ahci \
		-device ide-hd,drive=disk,bus=ahci.0 \
		-serial file:$(OS_LOG_DIR)/kernel.log
#	the loader enters the kernel at its LongMode label and writes into its
#	MEMORY_INFO and MEMORY_ENTRIES, so it needs to know where they are. we
#	read them out of kernel.elf every build instead of writing them down |
#	they move as soon as anything before them in kernel_entry.asm changes.
$(TEMP_DIR)/uefi_layout.h: $(OS_BUILD_DIR)/kernel.elf
	mkdir -p $(TEMP_DIR)
	@$(NM) $(OS_BUILD_DIR)/kernel.elf | python3 -c "\
import sys; \
base = 0xffffff8000008000; \
want = {'LongMode':'LONGMODE','GDT64.Pointer':'GDT64_POINTER','MEMORY_INFO':'MEMORY_INFO','MEMORY_ENTRIES':'MEMORY_ENTRIES'}; \
found = {}; \
[found.__setitem__(want[p[2]], int(p[0],16) - base) for p in (l.split() for l in sys.stdin) if len(p)==3 and p[2] in want]; \
print('#pragma once'); \
print('/*'); \
print('Generated by the makefile, do not edit | it reads the addresses out of'); \
print('kernel.elf with nm every time the kernel is built.'); \
print('*/'); \
[print('#define KERNEL_%-22s 0x%03X' % (k+'_OFFSET', found[k])) for k in ('LONGMODE','GDT64_POINTER','MEMORY_INFO','MEMORY_ENTRIES')]" \
	> $(TEMP_DIR)/uefi_layout.h
	@echo "[  OK  ] $(TEMP_DIR)/uefi_layout.h"

#	-fno-ident drops the .comment section, which the pe linker refuses to
#	place. -mno-red-zone because the firmware can interrupt us and the red
#	zone is a sysv idea it does not know about.
UEFI_CFLAGS = -I./include -I$(TEMP_DIR) -ffreestanding -fno-stack-protector \
	-fno-ident -fshort-wchar -mno-red-zone -std=gnu17 -Wall -Wextra

#	ld -mi386pep takes our elf objects and writes a PE32+ out of them, which
#	is why we do not need gnu-efi, clang or mingw | binutils is built with
#	--enable-targets=x86_64-pep, see install_toolbox.
#	subsystem 10 is EFI_APPLICATION.
uefi: $(TEMP_DIR)/uefi_layout.h
	mkdir -p $(OS_BUILD_DIR)
	$(CC) $(UEFI_CFLAGS) -c $(UEFI_SRC)/bootx64.c -o $(TEMP_DIR)/bootx64.o
	$(LD) -mi386pep --subsystem=10 -e efi_main --image-base=0x400000 -s \
		-o $(OS_BUILD_DIR)/BOOTX64.EFI $(TEMP_DIR)/bootx64.o
	@echo "[  OK  ] $(OS_BUILD_DIR)/BOOTX64.EFI"

#	the esp is a plain directory here. qemu serves it as a fat partition with
#	its vvfat driver, so we do not build an image just to test.
#	the firmware looks for /EFI/BOOT/BOOTX64.EFI and nothing else.
esp: uefi
	$(REMOVE) $(ESP_DIR)
	mkdir -p $(ESP_DIR)/EFI/BOOT
	cp $(OS_BUILD_DIR)/BOOTX64.EFI $(ESP_DIR)/EFI/BOOT/
	cp $(OS_BUILD_DIR)/kernel.bin $(ESP_DIR)/
	cp $(RAMFS_IMG) $(ESP_DIR)/
	@echo "[  OK  ] $(ESP_DIR)"

#	OVMF is the free uefi firmware, we need it to test without hardware.
#	the vars file has to be writable, so we copy it.
run_uefi: esp
	cp $(OVMF_VARS) $(OS_BUILD_DIR)/ovmf_vars.fd
	qemu-system-x86_64 -monitor stdio -m 4096 -no-reboot -no-shutdown \
		-drive if=pflash,format=raw,unit=0,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,unit=1,file=$(OS_BUILD_DIR)/ovmf_vars.fd \
		-drive file=fat:rw:$(ESP_DIR),format=raw \
		-serial file:$(OS_BUILD_DIR)/kernel_uefi.log

#	the same as run_uefi, only frozen at reset waiting for gdb on :1234.
#	it depends on esp, so the kernel the firmware loads is always the one the
#	symbols in kernel.elf describe | a stale esp debugs the previous build.
#
#	the firmware runs first here, which run_debug does not have to deal with :
#	a breakpoint on kernel_main is reached only after OVMF has handed over, so
#	setting one on efi_main needs BOOTX64.EFI and its own base address.
run_debug_efi: esp
	cp $(OVMF_VARS) $(OS_BUILD_DIR)/ovmf_vars_debug.fd
	qemu-system-x86_64 -s -S -monitor stdio -m 4096 -no-reboot -no-shutdown \
		-drive if=pflash,format=raw,unit=0,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,unit=1,file=$(OS_BUILD_DIR)/ovmf_vars_debug.fd \
		-drive file=fat:rw:$(ESP_DIR),format=raw \
		-serial file:$(OS_BUILD_DIR)/kernel_debug_uefi.log

#	The same thing on a q35, which is the machine to reach for when something
#	works here and not on the laptop.
#
#	The default machine is a i440fx from 1996 : it carries a legacy ide
#	controller, an isa bridge, all of it. A q35 has none of that, and neither
#	does a dell built this decade | the ata probe hanging on a floating bus
#	reproduced here exactly as it did on the hardware.
run_uefi_q35: esp
	cp $(OVMF_VARS) $(OS_BUILD_DIR)/ovmf_vars_q35.fd
	qemu-system-x86_64 -machine q35 -monitor stdio -m 4096 -no-reboot -no-shutdown \
		-drive if=pflash,format=raw,unit=0,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,unit=1,file=$(OS_BUILD_DIR)/ovmf_vars_q35.fd \
		-drive file=fat:rw:$(ESP_DIR),format=raw \
		-serial file:$(OS_BUILD_DIR)/kernel_q35.log


#	el torito, hard disk emulation : the bios emulates a disk out of the
#	image and our int 0x13 reads work in 512 byte sectors like everywhere
#	else. the no emulation mode would address the cd in 2048 byte sectors,
#	which _DiskLoad does not know how to do.
#	mkisofs warns that the partition does not start at chs 0/1/1, because
#	ours starts at lba 2048 like every modern tool aligns it. seabios does
#	not boot the result, so this is for a real cd drive or a vm, and the usb
#	target below is the one to use on real hardware.
iso:
	@if [ ! -f $(OS_IMAGE) ]; then \
		echo "[FAILED] $(OS_IMAGE) is missing, run make os first"; \
		exit 1; \
	fi
	$(REMOVE) $(ISO_ROOT)
	mkdir -p $(ISO_ROOT)/boot
	cp $(OS_IMAGE) $(ISO_ROOT)/boot/os-image
	mkisofs -quiet -R -J -V BALROGOS \
		-b boot/os-image -hard-disk-boot \
		-o $(OS_BUILD_DIR)/balrog.iso $(ISO_ROOT)
	@echo "[  OK  ] $(OS_BUILD_DIR)/balrog.iso"

########################################################
#	RELEASE
########################################################
RELEASE_DIR = build/release

#	the esp holds the ramfs, so it has to be bigger than it. 64MiB leaves room
#	for the ramfs to grow without touching this.
ESP_IMG_SIZE = 64

#	the partition starts at the sector 2048, the same place grub leaves it and
#	the same one src/Bootloader/common/layout.inc uses.
ESP_IMG_LBA = 2048

#	virtualbox has no usb mass storage controller of its own, so a usb key is
#	tested by attaching the very same image to a sata port : the firmware sees a
#	disk with a partition table either way, which is all our bootloaders read.
#	vbox wants a vdi rather than a raw file, so we hand it one when VBoxManage
#	is around.
VBOXMANAGE = VBoxManage

#	everything a bios machine needs. the raw image goes on a usb key with dd,
#	the vdi gets attached to a virtualbox sata port.
#
#	no iso here. make iso still builds one, but el torito hard disk emulation
#	never gets past our mbr : the bios hands stage 1 a geometry built from the
#	partition entry, and ours says 0xffffff on purpose because we read in lba.
release:
	$(MAKE) ramfs
	$(MAKE) bootloader
	$(MAKE) kernel
	$(MAKE) os
	mkdir -p $(RELEASE_DIR)
	cp $(OS_IMAGE) $(RELEASE_DIR)/balrog-bios.img
	@echo "[  OK  ] $(RELEASE_DIR)/balrog-bios.img  -> dd on a usb key"
	@if command -v $(VBOXMANAGE) > /dev/null; then \
		$(REMOVE) $(RELEASE_DIR)/balrog-bios.vdi; \
		$(VBOXMANAGE) convertfromraw $(RELEASE_DIR)/balrog-bios.img \
			$(RELEASE_DIR)/balrog-bios.vdi --format VDI > /dev/null; \
		echo "[  OK  ] $(RELEASE_DIR)/balrog-bios.vdi  -> a vbox sata port, EFI off"; \
	else \
		echo "[ INFO ] VBoxManage is missing, the raw image is still there"; \
	fi

#	the same thing for a machine that only boots uefi.
#
#	a bootable esp is a gpt disk carrying one fat32 partition with
#	/EFI/BOOT/BOOTX64.EFI in it, so we build the whole disk and not just the
#	tree : both a usb key and virtualbox want a partition table.
#
#	mkfs.vfat formats the partition in place with --offset, and mcopy fills it
#	through the same offset. -s walks the tree and makes EFI/BOOT on its own.
#	a loop mount would have needed root for what is an ordinary build step.
release_uefi:
	$(MAKE) ramfs
	$(MAKE) kernel
	$(MAKE) esp
	mkdir -p $(RELEASE_DIR)
	$(REMOVE) $(RELEASE_DIR)/balrog-uefi.img
	truncate -s $(ESP_IMG_SIZE)M $(RELEASE_DIR)/balrog-uefi.img
	printf 'label: gpt\n,,U\n' | $(SFDISK) $(RELEASE_DIR)/balrog-uefi.img > /dev/null
	$(MKFS_VFAT) -F 32 --offset $(ESP_IMG_LBA) -n BALROGOS \
		$(RELEASE_DIR)/balrog-uefi.img > /dev/null
	$(MCOPY) -s -i $(RELEASE_DIR)/balrog-uefi.img@@$$(( $(ESP_IMG_LBA) * 512 )) \
		$(ESP_DIR)/* ::/
	@echo "[  OK  ] $(RELEASE_DIR)/balrog-uefi.img  -> dd on a usb key"
	@if command -v $(VBOXMANAGE) > /dev/null; then \
		$(REMOVE) $(RELEASE_DIR)/balrog-uefi.vdi; \
		$(VBOXMANAGE) convertfromraw $(RELEASE_DIR)/balrog-uefi.img \
			$(RELEASE_DIR)/balrog-uefi.vdi --format VDI > /dev/null; \
		echo "[  OK  ] $(RELEASE_DIR)/balrog-uefi.vdi  -> a vbox sata port, EFI on"; \
	else \
		echo "[ INFO ] VBoxManage is missing, the raw image is still there"; \
	fi

debug:
	$(MAKE) clean
	$(MAKE) DEBUG=1 bootloader
	$(MAKE) DEBUG=1 kernel
	$(MAKE) DEBUG=1 os
	@echo "[  OK  ] symbols are in $(OS_BUILD_DIR)/kernel.elf, now run make run_debug"

run_debug:
#	same machine as `run`, only frozen at reset waiting for gdb on :1234.
#	the kernel only has an AHCI driver, a positional image lands on the
#	default IDE controller and no disk is seen at all.
	mv $(OS_LOG_DIR)/kernel_debug.log $(OS_LOG_DIR)/kernel_debug.log.bak | true
	qemu-system-x86_64 -s -S -monitor stdio -m 4096 -no-reboot -no-shutdown \
		-drive id=disk,file=build/os/os-image,format=raw,if=none \
		-device ahci,id=ahci \
		-device ide-hd,drive=disk,bus=ahci.0 \
		-serial file:$(OS_LOG_DIR)/kernel_debug.log

build_all:
	$(MAKE) tools
	$(MAKE) ramfs
	$(MAKE) bootloader
	$(MAKE) kernel
	$(MAKE) os
	@echo "[  OK  ] build os with tools"

build_all_debug:
	$(MAKE) clean
	$(MAKE) DEBUG=1 tools
	$(MAKE) DEBUG=1 ramfs
	$(MAKE) DEBUG=1 bootloader
	$(MAKE) DEBUG=1 kernel
	$(MAKE) DEBUG=1 esp
	$(MAKE) DEBUG=1 os
	@echo "[  OK  ] symbols are in $(OS_BUILD_DIR)/kernel.elf"
	@echo "         now run make run_debug or make run_debug_efi, then make gdb"

build:
	$(MAKE) bootloader
	$(MAKE) kernel
	$(MAKE) esp
	$(MAKE) os
	@echo "[  OK  ] build os without tools"

build_debug:
	$(MAKE) DEBUG=1 bootloader
	$(MAKE) DEBUG=1 kernel
	$(MAKE) DEBUG=1 esp
	$(MAKE) DEBUG=1 os
	@echo "[  OK  ] symbols are in $(OS_BUILD_DIR)/kernel.elf"
	@echo "         now run make run_debug or make run_debug_efi, then make gdb"

#	attach to the qemu left waiting by make run_debug.
#	CLion does the same thing through a Remote Debug configuration.
#	follow the serial log of a running kernel, or read the last one.
log:
	@tail -f $(OS_LOG_DIR)/kernel.log

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
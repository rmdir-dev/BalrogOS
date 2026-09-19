########################################################
#	PROJECT SETTINGS
########################################################
#	This file is here so CLion (or any cmake aware IDE) can resolve the
#	includes and navigate the code. It is NOT the build system :
#	the Makefile still builds the os. The libraries below are object
#	libraries, they are never linked, they only tell the IDE with which
#	flags every source file is compiled.
#
#	The flags, the defines and the include directories are the ones of
#	the Makefile. Keep them in sync.
########################################################
cmake_minimum_required(VERSION 3.16)
project(BalrogOS C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#	nasm and the .asm files are handled by the Makefile only, we don't
#	enable the ASM_NASM language here so the project still opens on a
#	machine without nasm installed.

########################################################
#	DIRECTORIES
########################################################
set(KERNEL_SRC		src/kernel)
set(KLIB_SRC		src/klib)
set(C_LIB_SRC		src/libc)
set(C_POSIX_SRC		src/posix)
set(SHARED_SRC		src/shared)
set(TOOLKIT_SRC		src/tool_kit)
set(UEFI_SRC		src/bootloader/uefi)

#	the Makefile writes uefi_layout.h in there and the loader includes it.
#	the file only shows up after a make uefi, the directory is in the include
#	path either way so the include resolves as soon as it has run.
set(TEMP_DIR		build/temp)

#	lai is a git submodule, the AML interpreter we evaluate _PTS with.
#	the directory is empty until git submodule update --init has run.
set(LAI_SRC			lai)

include_directories(
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/include/libc
	${CMAKE_SOURCE_DIR}/include/posix
	${CMAKE_SOURCE_DIR}/${LAI_SRC}/include
	${CMAKE_SOURCE_DIR}/${TEMP_DIR}
)

########################################################
#	DEFINES
########################################################
add_compile_definitions(
	KDB_DEBUG
	KDB_DEFAULT_LVL=5
	KDB_START_SEQ=0
	__BALROG_VERSION__="0.0.1"
)

########################################################
#	COMPILER FLAGS
########################################################
add_compile_options(
	-ffreestanding
	-nostdlib
	-mno-red-zone
	-Werror=return-type
	-Werror=implicit-int
	-Wno-address-of-packed-member

	# gcc 14 turned these three warnings into errors. the kernel relies on
	# the old behaviour in a lot of places, so they are put back to warnings.
	-Wno-error=incompatible-pointer-types
	-Wno-error=int-conversion
	-Wno-error=implicit-function-declaration
)

########################################################
#	SOURCE FILES
########################################################
# shared between the kernel and the libc
file(GLOB_RECURSE SHARED_SRCS	CONFIGURE_DEPENDS ${SHARED_SRC}/*.c)

# Kernel
file(GLOB_RECURSE C_SRCS		CONFIGURE_DEPENDS ${KERNEL_SRC}/*.c ${KLIB_SRC}/*.c)

# lai, third party. core is the interpreter itself, helpers holds
# lai_enter_sleep, drivers holds the embedded controller _PTS talks to.
file(GLOB_RECURSE LAI_SRCS		CONFIGURE_DEPENDS
	${LAI_SRC}/core/*.c ${LAI_SRC}/helpers/*.c ${LAI_SRC}/drivers/*.c)

# libc
file(GLOB_RECURSE LIBC_SRCS		CONFIGURE_DEPENDS ${C_LIB_SRC}/*.c)

# pthread
file(GLOB_RECURSE PTHREADC_SRCS	CONFIGURE_DEPENDS ${C_POSIX_SRC}/*.c)

# tools
file(GLOB_RECURSE TOOLS_SRCS	CONFIGURE_DEPENDS ${TOOLKIT_SRC}/*.c)

# headers, so the ones no source includes yet are still indexed
file(GLOB_RECURSE ALL_HEADERS	CONFIGURE_DEPENDS include/*.h)

########################################################
#	INDEXING TARGETS
########################################################
#	the headers are listed here and nowhere else. a header that no source
#	includes yet has no translation unit to take its flags from, listing it
#	gives it one | listing it in two targets only makes CLion pick one of
#	them at random, so kernel owns them all, the flags are the same anyway.
add_library(kernel		OBJECT ${C_SRCS} ${SHARED_SRCS} ${ALL_HEADERS}
		src/klib/data_structure/vector.c)

#	its own target, so we don't mix its warnings with ours. CLion still
#	indexes lai_enter_sleep and the rest of it.
if(LAI_SRCS)
	add_library(lai		OBJECT ${LAI_SRCS})
endif()
add_library(libc		OBJECT ${LIBC_SRCS} ${SHARED_SRCS})
add_library(libpthread	OBJECT ${PTHREADC_SRCS})
add_library(tools		OBJECT ${TOOLS_SRCS})

#	the loader is built on its own by the Makefile, with its own flags, so it
#	needs its own target here too | without it bootx64.c belongs to nothing
#	and the boot/ headers have no context at all.
#	the shared sources it links are left out on purpose, they are already
#	indexed in kernel and a file in two targets is a file CLion has to guess.
add_library(uefi		OBJECT ${UEFI_SRC}/bootx64.c)
target_compile_options(uefi PRIVATE
	-fshort-wchar
	-fno-stack-protector
	-mno-red-zone
)

########################################################
#	BUILD (delegates to the Makefile)
########################################################
add_custom_target(BalrogOS
	COMMAND make bootloader
	COMMAND make kernel
	COMMAND make os
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	USES_TERMINAL
	COMMENT "building the os image"
)

add_custom_target(BalrogOS_tools
	COMMAND make tools
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	USES_TERMINAL
	COMMENT "building the tool-kit and copying it into the disk image"
)

add_custom_target(BalrogOS_run
	COMMAND make run
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	USES_TERMINAL
	COMMENT "running the os in qemu"
)

add_custom_target(BalrogOS_run_debug
	COMMAND make run_debug
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	USES_TERMINAL
	COMMENT "running the os in qemu, stopped, gdb on localhost:1234"
)

add_custom_target(BalrogOS_clean
	COMMAND make clean
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	USES_TERMINAL
	COMMENT "removing build/"
)

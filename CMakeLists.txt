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
set(KERNEL_SRC		src/Kernel)
set(KLIB_SRC		src/klib)
set(C_LIB_SRC		src/Libc)
set(C_POSIX_SRC		src/POSIX)
set(SHARED_SRC		src/Shared)
set(TOOLKIT_SRC		src/tool-kit)

include_directories(
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/include/libc
	${CMAKE_SOURCE_DIR}/include/POSIX
)

########################################################
#	DEFINES
########################################################
add_compile_definitions(
	KDB_DEBUG
	KDB_DEFAULT_LVL=3
	KDB_START_SEQ=0
	__BALROG_VERSION__="0.0.1"
)

########################################################
#	COMPILER FLAGS
########################################################
add_compile_options(
	-ffreestanding
	-nostdlib
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
add_library(kernel		OBJECT EXCLUDE_FROM_ALL ${C_SRCS} ${SHARED_SRCS} ${ALL_HEADERS})
add_library(libc		OBJECT EXCLUDE_FROM_ALL ${LIBC_SRCS} ${SHARED_SRCS})
add_library(libpthread	OBJECT EXCLUDE_FROM_ALL ${PTHREADC_SRCS})
add_library(tools		OBJECT EXCLUDE_FROM_ALL ${TOOLS_SRCS})

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

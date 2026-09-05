[org 0x0500]                ; stage 1 loads us here, not at 0x7e00 any more.
                            ; from here up to the boot sector at 0x7c00 there is
                            ; about 30KiB, instead of the 512 bytes the old
                            ; second sector had between 0x7e00 and the kernel.

[bits 16]                   ; set the mode as 16 bit real mode

%include "src/bootloader/common/layout.inc"        ; include the layout shared with stage 1
                                            ; and the makefile.
%include "src/bootloader/bios/io/bios/io.inc"    ; include the macros

; _DiskLoad takes its sector count in dh, which is a byte, so a read longer
; than 255 sectors has to be split. the loops below step by CHUNK_SECTORS and
; stop when the counter reaches zero, which only works if the totals divide
; evenly. checking it here turns a boot that never ends into a build error.
%if (KERNEL_SECTORS % CHUNK_SECTORS) != 0
    %error "KERNEL_SECTORS must be a multiple of CHUNK_SECTORS"
%endif
%if (RAMFS_SECTORS % CHUNK_SECTORS) != 0
    %error "RAMFS_SECTORS must be a multiple of CHUNK_SECTORS"
%endif

section .text
    global _Stage2Start

_Stage2Start:
    mov [BOOT_DRIVE], dl    ; stage 1 hands the boot drive over in dl, the same
                            ; way the bios handed it to stage 1.

    PrintStringNextLine STAGE2_MSG

    call _TestA20           ; enables the line if the test says it is off
    call _CheckLongMode     ; stops here if the cpu has no long mode, rather
                            ; than three mode switches later

    call _DetectMemorySize  ; the e820 map is read first, while the machine is
                            ; still plain real mode with no unreal descriptor
                            ; caches and nothing loaded over anything.
                            ; int 0x15 fails silently, the map just comes back
                            ; empty, so it gets the cleanest state we have.

    call _LoadKernel        ; the kernel goes low, at KERNEL_ADDR, where a
                            ; plain real mode segment reaches it anyway.
    PrintStringNextLine KERNEL_MSG

    call _EnterUnrealMode   ; ds and es can reach 4GiB from here on.
                            ; only _LoadRamfs below needs it, so nothing runs
                            ; in this mode that does not have to.

    call _LoadRamfs         ; the ramfs goes to RAMFS_PHYS, far above 1MiB.
                            ; the bios cannot write up there, so each chunk
                            ; lands in a low buffer and is copied up with a
                            ; 32bit move.
    PrintStringNextLine RAMFS_MSG

    mov ax, MEMORY_SIZE_KB  ; _PrepareKernel reads both pointers back out of
    mov bx, MEMORY_ENTRY_COUNT  ; ax and bx, see kernel_entry.asm
    call KERNEL_ADDR        ; _PrepareKernel, which never returns

    jmp $                   ; should never be reached

STAGE2_MSG:
    db "stage 2",0
KERNEL_MSG:
    db "kernel",0
RAMFS_MSG:
    db "ramfs",0

%include "src/bootloader/bios/io/bios/print.asm"
%include "src/bootloader/bios/io/bios/disk.asm"
%include "src/bootloader/bios/a20/a20.asm"
%include "src/bootloader/bios/unreal_mode/unreal_mode.asm"
%include "src/bootloader/bios/long_mode_x64/long_mode.asm"
%include "src/bootloader/bios/memory/memory.asm"

BOOT_DRIVE:
    db 0

    times (STAGE2_SECTORS * 512)-($-$$) db 0    ; pad stage 2 to a fixed size, so
                                                ; KERNEL_LBA never moves when the
                                                ; code in here grows.

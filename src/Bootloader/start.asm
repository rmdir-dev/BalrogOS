[org 0x7c00]                ; say that the code is at that memory address
                            ; if we don't do that, when using labels the cpu will think
                            ; that our base address is 0x0000 in memory and will try access
                            ; labels using that address as the base for the offset.
                            ; so when we try to use a label, it's memory address will be 
                            ; 0x0000 + label offset. But the bios load our code at 0x7c00
                            ; which would make the resulting address incorrect.
                            ; The correct address would be 0x7c00 + label offset.
                            ; so this says that our base address is 0x7c00 and to offset
                            ; everything based on that address

[bits 16]                   ; set the mode as 16 bit real mode

%include "src/Bootloader/IO/BIOS/io.inc" ; include the macros

%define KERNEL_OFFSET 0xFFFFFF8000000000

section .text
    global _start

_start:
    cli                     ; disable interrupt
                            ; to be sure that no interrupt are called when we're resetting
                            ; our registers and segments.
    jmp 0x0000:init
init:
    xor ax, ax              ; clearing register ax
    mov ss, ax              ; clearing segments
    mov ds, ax              ; clearing segments
    mov es, ax              ; clearing segments
    mov fs, ax              ; clearing segments
    mov gs, ax              ; clearing segments

    
    cld                     ; clear direction flag (string read directiom)
                            ; cld clear the flag and set it to 0, to be sure we're reading
                            ; strings from left to right

    sti                     ; enable interrupt

    push ax                 ; save ax
    xor ax, ax              ; clear ax
    int 0x13                ; Reset the disk to be sure that we're starting at the begining

    mov ah, 0x01            ; disabling the cursor
    mov ch, 0x3f            ;
    int 0x10                ;

    pop ax                  ; recover ax

    mov [BOOT_DRIVE], dl    ; the bios store our boot drive id into dl
                            ; so we store it into BOOT_DRIVE to be able to use it.

    PrintStringNextLine MSG ; print MSG

    ;PrintHexToLine 0x2af5   ; print an hex number

    mov dl, [BOOT_DRIVE]    ; put the boot drive into dl, to say we want to read it.
    mov dh, 1               ; we want to read 1 sectors from it
    mov cl, 0x02            ; read sector 2
    mov bx, 0x0000          ; higher word of the memory address we want to store our data to
    mov es, bx              ; set the higher word of the address into es
    mov bx, 0x7c00 + 512    ; lower word of the memory addres into bx
                            ; 0x200 = 512
                            ; BIOS will store data at address es:bx
                            ; so here 0x00007e00
    call _DiskLoad          ; load the disk data

    mov dl, [BOOT_DRIVE]    ; put the boot drive into dl, to say we want to read it.
    mov dh, 9               ; we want to read 1 sectors from it
    mov cl, 0x03            ; read sector 3
    mov bx, 0x0000          ; higher word of the memory address we want to store our data to
    mov es, bx              ; set the higher word of the address into es
    mov bx, 0x8000          ; lower word of the memory addres into bx
                            ; 0x200 = 512
                            ; BIOS will store data at address es:bx
                            ; so here 0x00008000
    call _DiskLoad          ; load the disk data

    mov esp, init - KERNEL_OFFSET ; set the stack pointer to main

    call 0x7c00 + 512       ; calling sector 2 function

    jmp $                   ; infinite loop that jump to our current location in memory
                            ; $ = our current location in memory.

%include "src/Bootloader/IO/BIOS/print.asm"
%include "src/Bootloader/IO/BIOS/disk.asm"

BOOT_DRIVE:
    db 0

MSG:    
    db "Loading...",0

    times 510-($-$$) db 0   ; here we fill the rest of our bootloader with 0
                            ; the bootloader size is 512, the last 2 bytes must be 0xaa55
                            ; so here we say that it must set 510 - (our current location - boot loader start location)
                            ; eg: 510 - (0x7c04 - 0x7c00) = 510 - 4
                            ; so it would set 506 bytes to 0

    dw 0xaa55               ; this is the magic number.
                            ; when your cpu is looking for a bootloader,
                            ; it will look if there is this number at the end of the first sector

_SectorTwoProgram:
    call _TestA20
    call _CheckLongMode

    jmp 0x8000


%include "src/Bootloader/LongMode_x64/longMode.asm"
%include "src/Bootloader/A20/A20.asm"

    times 512-($-$$-512) db 0
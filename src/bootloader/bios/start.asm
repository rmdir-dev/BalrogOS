[org 0x7c00]                ; say that the code is at that memory address
                            ; if we don't do that, when using labels the assembler will think
                            ; that our base address is 0x0000 in memory and will try access
                            ; labels using that address as the base for the offset.
                            ; so when we try to use a label, it's memory address will be 
                            ; 0x0000 + label offset. But the bios load our code at 0x7c00
                            ; which would make the resulting address incorrect.
                            ; The correct address would be 0x7c00 + label offset.
                            ; so this says that our base address is 0x7c00 and to offset
                            ; everything based on that address

[bits 16]                   ; set the mode as 16 bit real mode

%include "src/bootloader/common/layout.inc"         ; include the layout shared with stage 2 and the makefile
                                                    ; used for memory mapping and loading addresses.
%include "src/bootloader/bios/io/bios/io.inc"       ; include the macros

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
    mov sp, 0x7c00          ; set stack pointer to the bootloader base address
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

    mov dl, [BOOT_DRIVE]    ; put the boot drive into dl, to say we want to read it.
    mov dh, STAGE2_SECTORS  ; we want to read STAGE2_SECTORS sectors from it
                            ; 16 * 512B = 8KiB (the value lives in layout.inc)
                            ; dh is a byte, so this must stay under 256
    mov di, STAGE2_LBA      ; read sector 1, the one right behind this one.
                            ; stage 2 is written immediately after the boot
                            ; sector, so its lba is a constant : it doesn't move
                            ; when the kernel or the ramfs grow behind it.
                            ; the kernel is not loaded here any more, it sits at
                            ; KERNEL_LBA and stage 2 goes and gets it. that is
                            ; what removes the 65KiB chaining this comment used
                            ; to describe, and with it the 0x1ea0 / 0x1f20 pair
                            ; that had to be kept in sync with the makefile.
    mov bx, 0x0000          ; higher word of the memory address we want to store our data to
    mov es, bx              ; set the higher word of the address into es
    mov bx, STAGE2_ADDR     ; lower word of the memory addres into bx
                            ; BIOS will store data at address es:bx
                            ; so here 0x00000500, the first free byte above the
                            ; bios data area. from there up to this sector at
                            ; 0x7c00 there is about 30KiB, against the 512 bytes
                            ; the old second sector had at 0x7e00 before running
                            ; into the kernel at 0x8000.
                            
    call _DiskLoad          ; load the disk data

    mov dl, [BOOT_DRIVE]    ; set the boot device the same way BIOS gave it to us
                            ; the idea is that stage 2 should receive the regs with the right values.
                            ; currently this line is redundant, but it is there to avoid breaking anything
                            ; if a change _DiskLoad change dl's value.

    jmp 0x0000:STAGE2_ADDR  ; jumping into stage2

%include "src/bootloader/bios/io/bios/print.asm"
%include "src/bootloader/bios/io/bios/disk.asm"

BOOT_DRIVE:
    db 0

    times 446-($-$$) db 0   ; here we fill the rest of our bootloader with 0.
                            ; the partition table starts at offset 446, not 510 :
                            ; the last 66 bytes of the sector are 4 partition
                            ; entries of 16 bytes plus the 2 magic ones.
                            ; eg: 446 - (0x7c04 - 0x7c00) = 446 - 4
                            ; so it would set 442 bytes to 0

; the partition table. the bootloader never reads it, it works from the lbas in
; layout.inc. it is written for everything else : a bios booting off a usb key
; checks it to decide the disk is bootable, and any tool that touches this disk
; reads it to know which regions are taken.
PARTITION_TABLE:
;   partition 1 : the kernel and the ramfs, raw
    db 0x80                 ; bootable flag, 0x80 = active
    db 0xff, 0xff, 0xff     ; CHS of the first sector.
                            ; 0xffffff is the "too far for CHS, use lba" marker
                            ; every tool writes for a partition past the 8GiB
                            ; CHS ceiling, and ours is past it by convention.
    db 0xda                 ; partition type 0xda = non-fs data. there is no
                            ; filesystem in here, just a kernel and a ramfs
                            ; image at the offsets layout.inc gives them.
    db 0xff, 0xff, 0xff     ; CHS of the last sector, same marker
    dd PART1_LBA            ; first sector, in lba
    dd PART1_SECTORS        ; how many sectors it holds

;   partitions 2 to 4 : empty. an all zero entry is what "unused" looks like.
    times 16 * 3 db 0

    dw 0xaa55               ; this is the magic number.
                            ; when your cpu is looking for a bootloader,
                            ; it will look if there is this number at the end of the first sector
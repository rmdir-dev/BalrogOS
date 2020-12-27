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

    mov sp, init            ; set the stack pointer to main
    cld                     ; clear direction flag (string read directiom)
                            ; cld clear the flag and set it to 0, to be sure we're reading
                            ; strings from left to right

    sti                     ; enable interrupt

    push ax                 ; save ax
    xor ax, ax              ; clear ax
    int 0x13                ; Reset the disk to be sure that we're starting at the begining
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

    call _TestA20

    call _SectorTwoProgram  ; calling sector 2 function

    jmp $                   ; infinite loop that jump to our current location in memory
                            ; $ = our current location in memory.

%include "src/Bootloader/IO/BIOS/print.asm"
%include "src/Bootloader/IO/BIOS/disk.asm"
%include "src/Bootloader/A20/A20.asm"

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
    call _CheckLongMode

    cli                     ; disable interrupt

    mov edi, 0x1000         ; set the destination index to 0x1000
    mov cr3, edi            ; set control register 3 to destination
    xor eax, eax            ; nullify eax
    mov ecx, 4096           ; set ecx to 4096 (will be use as a counter)
    rep stosd               ; clear the memory from 0x1000 to 0x5000
    mov edi, 0x1000         ; set edi back to 0x1000 (PML4T)

    ; PAGING                                                    total paging cover 256TiB of memory
    ; PML4T address 0x1000 pointing to PDPT                     each PML4T hold 512GiB
    ; PDPT  address 0x2000 pointing to PDT                      each PDPT hold 1GiB
    ; PDT   address 0x3000 pointing to PT                       each PDT hold 2MiB
    ; PT    address 0x4000 pointing to the pages                each PT hold 4kiB (pages)

    mov dword [edi], 0x2003     ; Set the addres of the begining of PDPT to the first address of PML4T
                                ; the two first bytes are the pointer to the next table
                                ; 2003 3 is for 0b11 = present and writable
    add edi, 0x1000             ; add 0x1000 to edi so now edi point to 0x2000
    mov dword [edi], 0x3003     ; Set the addres of the begining of PDPT to PDT
    add edi, 0x1000             ; add 0x1000 to edi so now edi point to 0x3000
    mov dword [edi], 0x4003     ; Set the addres of the begining of PDT to PT
    add edi, 0x1000             ; add 0x1000 to edi so now edi point to 0x4000

    mov dword ebx, 0x00000003   ; ebx to 0x00000003 3 = present and writable
    mov ecx, 512                ; ecx to 512 will be use as counter

.SetEntry:
    mov dword [edi], ebx        ; set ebx into edi
    add ebx, 0x1000             ; add 0x1000 to ebx
    add edi, 8                  ; add 8 to edi (shift 8 byte so to the next address)
    loop .SetEntry              ; loop while ecx is not equal to 0
                                ; the map the first 2MB of memory
                                ; so it will map the memory from 0x00000000 to 0x00200000

    mov eax, cr4                ; set the cr4 register to eax
    or eax, 1 << 5              ; set the PAE-bit to 1
                                ; physical address extension
                                ; If set, changes page table layout to translate 32-bit virtual addresses 
                                ; into extended 36-bit physical addresses.
                                ; enable the PAE paging
    mov cr4, eax                ; set the new value to cr4

    mov ecx, 0xC0000080         ; set ecx to EFER MSR
    rdmsr                       ; read from the model specific register
    or eax, 1 << 8              ; set the LM-bit to 1 (long mode = x64)
    wrmsr                       ; write to msr

    mov eax, cr0                ; set eax to cr0
    or eax, 1 << 31 | 1 << 0    ; set the pg-bit and the pe-bit
                                ; pg-bit enable paging
                                ;   - to 0 = paging disable
                                ;   - to 1 = paging enable and use CR3 register
                                ; pe-bit enable protected mode
                                ;   - to 0 = real mode
                                ;   - to 1 = protected mode
    mov cr0, eax                ; set the new value to cr0

    lgdt [GDT64.Pointer]        ; load gdt

    jmp GDT64.Code:LongMode

%include "src/Bootloader/LongMode_x64/longMode.asm"
%include "src/Bootloader/LongMode_x64/gdt.asm"

[bits 64]                       ; switching to 64bit

LongMode:
    %define VID_MEM 0xb8000

    mov edi, VID_MEM
    mov eax, 0x0f20
    mov ecx, 2000

.clearLoop:
    call _printChar
    loop .clearLoop
    mov esi, IN_x64_PROTECTED_MODE_MSG
    call _SysPrintString

    jmp 0x8000
    hlt                         ; halt
    sti                         ; enable interrupt
    ret


%include "src/Bootloader/IO/System/print.asm"

IN_x64_PROTECTED_MODE_MSG:
    db "[LOAD] x64 protected mode",0

    times 512-($-$$-512) db 0
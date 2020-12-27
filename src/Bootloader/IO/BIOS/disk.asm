; INPUT
; dl = drive ID
; dh = number of sector to read
; cl = sector
; bx = lower address word
; es = higher address word
; es:bx = address to write to
; eg: es = 0x000f bx = 0x9521
; eg: address is 0x000f9521
_DiskLoad:
    pusha               ; push everything register to the stack
    push dx             ; push dx to the stack
                        ; dx contain the number of sectors to read

    mov ah, 0x02        ; INT 0x13, AH = 2 -- read floppy/hard disk in CHS mode
    mov al, dh          ; read dh sector
    mov ch, 0x00        ; cylinder 0
    mov dh, 0x00        ; head 0
    ;mov cl, 0x02        ; sector 2
                        ; after the boot sector
    int 0x13            ; bios mass storage (disk, floppy) access interrupt
                        ; INPUT
                        ; AH = 0x02 = read sectors from drive CSH
                        ; AL = sectors count
                        ; CH = cylinder
                        ; cl = sector
                        ; dh = head
                        ; dl = drive
                        ; es:bx = buffer address to write into.
                        ; RESULT
                        ; CF = set on if the root sector does not exist.
                        ; ah = return code
                        ; al = number of sector read

    jc .diskError       ; if the interrupt failed to read the first sector
                        ; jc = jump if carry flag is set
    pop dx              ; recover dx containing the number of sector to read
    cmp dh, al          ; compare if the number of sector read is equal to the requested ones
    jne .sectorReadError; if not jump to diskError

    popa                ; recover every register from the stack.
    ret

.sectorReadError:
    PrintStringNextLine DISK_SECTOR_ERROR_STRING
    jmp $

.diskError:
    PrintStringNextLine DISK_ERROR_STRING   ; declared in io.inc included into start.asm
    jmp $

DISK_SECTOR_ERROR_STRING:
    db "Sect Err read disk!",0

DISK_ERROR_STRING:
    db "Err read disk!",0
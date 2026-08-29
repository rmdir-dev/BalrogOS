; recently upgrade with :
; - https://wiki.osdev.org/Disk_access_using_the_BIOS_(INT_13h) : Reading sectors with a CHS address, LBA in Extended Mode.
; - https://www.ctyme.com/intr/rb-0621.htm : Int 13/AH=08h - DISK - GET DRIVE PARAMETERS.
; - https://www.ctyme.com/intr/rb-0607.htm : Int 13/AH=02h - DISK - READ SECTOR(S) INTO MEMORY.
; - https://www.ctyme.com/intr/rb-0708.htm : Int 13/AH=42h - IBM/MS INT 13 Extensions - EXTENDED READ.

; INPUT
; dl = drive ID
; dh = number of sector to read
; di = start sector
; bx = lower address word
; es = higher address word
; es:bx = address to write to
; eg: es = 0xf000 bx = 0x9521
; eg: address is 0x000f9521
_DiskLoad:
    pusha               ; push everything register to the stack
    push es             ; int 0x13 is allowed to come back with es:di
                        ; changed, so the destination segment is saved too

    mov [DAP_SEGMENT], es   ; the packet carries the destination itself
    mov [DAP_OFFSET], bx
    mov [DAP_LBA], di   ; the packet holds the lba on 64 bits,
                        ; the upper words are left to 0
    mov cl, dh          ; cl = number of sectors still to read

.readChunk:
    ; the bios reads through the dma controller, and a single transfer
    ; can't cross a 64KiB boundary. so we only ask for what fits until
    ; the next one and come back for the rest.
    mov ax, [DAP_SEGMENT]
    shl ax, 4
    add ax, [DAP_OFFSET]; ax = the address inside its own 64KiB block
    neg ax              ; how many bytes are left before the boundary
    shr ax, 9           ; the same, counted in sectors
    jnz .clampToLeft
    mov ax, 127         ; we sit on a boundary, the bios takes 127 at once

.clampToLeft:
    cmp al, cl
    jbe .readSectors
    mov al, cl          ; never ask for more than what is left to read

.readSectors:
    xor ah, ah
    mov [DAP_COUNT], ax

    push dx             ; dl holds the drive, int 0x13 must not lose it
    mov ah, 0x42        ; INT 0x13, AH = 0x42 -- read the disk in LBA mode
    mov si, DISK_ADDRESS_PACKET
    int 0x13            ; bios mass storage (disk, floppy) access interrupt
                        ; INPUT
                        ; AH = 0x42 = extended read
                        ; DL = drive
                        ; DS:SI = disk address packet
                        ; RESULT
                        ; CF = set on if the read failed
                        ; ah = return code
    pop dx              ; pop leaves the flags alone

    jc .diskError       ; if the interrupt failed to read the sectors
                        ; jc = jump if carry flag is set

    mov ax, [DAP_COUNT] ; walk the destination and the lba forward by
    sub cl, al          ; what has just been read
    add [DAP_LBA], ax
    adc word [DAP_LBA + 2], 0
    shl ax, 5           ; a sector is 512 bytes, so 32 paragraphs
    add [DAP_SEGMENT], ax   ; the offset never moves, only the segment does

    cmp cl, 0           ; keep going until everything has been read
    jne .readChunk

    pop es              ; recover the destination segment
    popa                ; recover every register from the stack.
    ret

.diskError:
    PrintStringNextLine DISK_ERROR_STRING   ; declared in io.inc included into start.asm
    jmp $

DISK_ERROR_STRING:
    db "Read",0

; the bios reads the description of the transfer from this packet.
DISK_ADDRESS_PACKET:
    db 0x10             ; size of the packet
    db 0x00             ; reserved
DAP_COUNT:
    dw 0x0000           ; number of sectors to transfer
DAP_OFFSET:
    dw 0x0000           ; destination offset
DAP_SEGMENT:
    dw 0x0000           ; destination segment
DAP_LBA:
    dq 0x0000           ; first sector to read

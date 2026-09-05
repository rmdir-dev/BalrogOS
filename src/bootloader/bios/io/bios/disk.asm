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

; INPUT none, everything comes from layout.inc
; reads the kernel from KERNEL_LBA to KERNEL_ADDR, in chunks because
; KERNEL_SECTORS does not fit in the dh _DiskLoad reads its count from.
; the destination is under 1MiB, so the bios writes straight into it and
; there is nothing to copy afterwards.
_LoadKernel:
    mov ecx, KERNEL_SECTORS ; the total number of sectors, from layout.inc
    mov si, KERNEL_LBA  ; si carries the lba, because di goes to _DiskLoad
    mov bp, KERNEL_ADDR >> 4    ; the destination as a segment, 0x8000 -> 0x0800

.nextChunk:
    mov dl, [BOOT_DRIVE]    ; put the boot drive into dl, to say we want to read it.
    mov dh, CHUNK_SECTORS   ; CHUNK_SECTORS * 512B = 32KiB for this pass
    mov di, si          ; _DiskLoad takes the start sector in di
    mov bx, bp          ; higher word of the memory address we want to store
    mov es, bx          ; our data to, so the kernel segment
    xor bx, bx          ; lower word of the memory addres into bx

    call _DiskLoad      ; load the disk data
                        ; it prints and halts by itself if the read fails

    add si, CHUNK_SECTORS   ; walk the lba forward by what has just been read
    add bp, CHUNK_SECTORS * 32  ; a sector is 512 bytes, so 32 paragraphs
    sub ecx, CHUNK_SECTORS  ; and count it off the total
    jnz .nextChunk      ; keep going until everything has been read
    ret

; INPUT none, everything comes from layout.inc
_LoadRamfs:
    mov ecx, RAMFS_SECTORS  ; the total number of sectors, from layout.inc
    mov edi, RAMFS_PHYS ; the linear destination, it walks forward by itself
    mov bp, RAMFS_LBA   ; bp carries the lba, and it has to be bp : di and si
                        ; both belong to the movsd below, which walks them
                        ; forward by 32KiB on every pass.

.nextChunk:
    push ecx            ; save the number of sectors left to read
    push edi            ; di is clobbered right below, and the popa inside
                        ; _DiskLoad only restores its low 16 bits.

    mov dl, [BOOT_DRIVE]    ; put the boot drive into dl, to say we want to read it.
    mov dh, CHUNK_SECTORS   ; CHUNK_SECTORS * 512B = 32KiB for this pass
    mov di, bp          ; _DiskLoad takes the start sector in di
    mov bx, LOAD_BUFFER_SEG ; higher word of the memory address we want to store
    mov es, bx          ; our data to, so the low buffer under 1MiB
    xor bx, bx          ; lower word of the memory addres into bx
                        ; the buffer is 64KiB aligned so the offset is 0

    call _DiskLoad      ; load the disk data
                        ; it prints and halts by itself if the read fails

    pop edi             ; recover the linear destination

    push ds             ; save ds, the copy below needs it flat
    xor ax, ax          ; clear ax
    mov ds, ax          ; ds and es both flat. their descriptor cache still
    mov es, ax          ; holds the 4GiB limit from _EnterUnrealMode

    mov esi, LOAD_BUFFER_SEG << 4   ; the buffer, as a linear address
    mov ecx, CHUNK_SECTORS * 512 / 4    ; the chunk, counted in dwords
    a32 rep movsd       ; a32 forces 32bit addressing on a 16bit instruction.
                        ; this is the whole point of unreal mode : esi and
                        ; edi span 4GiB instead of the 64KiB a real mode
                        ; segment can see.
                        ; movsd walks edi forward on its own, so there is
                        ; nothing to add to it afterwards.
    pop ds              ; recover ds

    pop ecx             ; recover the number of sectors left to read
    add bp, CHUNK_SECTORS   ; walk the lba forward by what has just been read
    sub ecx, CHUNK_SECTORS  ; and count it off the total
    jnz .nextChunk      ; keep going until everything has been read
    ret

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

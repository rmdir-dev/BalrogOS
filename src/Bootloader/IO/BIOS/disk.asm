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
    push bx             ; save es bx
    push dx             ; push dx to the stack
                        ; dx contain the number of sectors to read
    mov ah, 0x08        ; get disk info
    int 0x13            ;
                        ; return 
                        ; Set AH to 8, DL to the BIOS drive number, and execute INT 0x13.
                        ; The value returned in DH is the "Number of Heads" -1.
                        ; AND the value returned in CL with 0x3f to get the "Sectors per Track".
                        ; 0x3f sec per track
                        ; 0x10 head

    xor bx, bx
    mov bl, dh          ; number of head into bx
    mov ax, 0x3f
    and cx, ax
    ; Temp = LBA / (Sectors per Track)
    ; temp = 140 V
    ; temp = 124 if b == 0x10
    xor dx, dx
    mov ax, di
    div cx          ; cx / ax (LBA/ SEC_PER_TRACK)
    
    push ax ; TEMP
    ; Sector = (LBA % (Sectors per Track)) + 1
    ; sec = 28
    xor dx, dx
    mov ax, di
    div cx
    
    inc dx
    pop ax  ; TEMP
    
    push dx ; SECTOR
    push ax ; TEMP
    ; Head = Temp % (Number of Heads)
    ; head = 12 V
    xor dx, dx
    inc bx

    div bx  ; 
    
    pop ax  ; temp
    push dx ; HEAD
    ; Cylinder = Temp / (Number of Heads)
    ; cylinder = 7 V
    xor dx, dx
    div bx
    mov ch, al ; CYLINDER
    pop ax
    mov di, ax ; HEAD
    pop ax
    mov cl, al ; SECTOR
    pop dx     ; pop sector count and drive id
    mov al, dh ; sector count
    pop bx
    push dx
    push bx
    mov bx, di
    mov dh, bl
    pop bx
      
    mov ah, 0x02        ; INT 0x13, AH = 2 -- read floppy/hard disk in CHS mode
    ; al = 128 MAX  V
    ; dl = 0x80
    ; cl = 1   V   SECTOR   29
    ; dh = 0   X   HEAD     12
    ; ch = 20  X   CYLINDER 7
    
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
    db "Sec disk",0

DISK_ERROR_STRING:
    db "Read",0
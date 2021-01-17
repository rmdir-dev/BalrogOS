section .text

_DetectMemorySize:
    pusha

    mov di, 0x0000
    mov es, di
    mov di, MEMORY_SIZE_KB      ; set di to 0x8004 to avoid being stuck in interrupt 0x15
         
    xor ebx, ebx                ; clear ebx must be set to 0 for 0x15 ax 0xe820 first call 
                                ; bx is the continuation value, it is use to get the next 
                                ; run of physical memory.
    xor bp, bp                  ; clear bp, will be use to know how many entry were passed.

    mov edx, 0x534d4150         ; Set edx to "SMAP"
                                ; S = 0x53
                                ; M = 0x4d
                                ; A = 0x41
                                ; P = 0x50
    
    mov eax, 0xe820             ; place the interrupt option to 0xe820
                                ; get complete memory map

    mov [es:di + 20], dword 1   ; force a valid ACPI 3.x entry
    mov ecx, 24                 ; ask for 24 byte output
    int 0x15                    ; interrupt 0x15 memory size functions
    jc .failed                  ; if carry flag set on first call = "unsupported function"

    mov edx, 0x534d4150         ; Set edx to "SMAP"
    cmp eax, edx                ; int 0x15 should set eax to "SMAP"
    jne .failed                 ;

    test ebx, ebx               ; if ebx = 0 -> list is only 1 entry long (useless)
    je .failed                  ;
    jmp .check_output           ;

.e820_loop:
    mov eax, 0xe820             ; 
    mov [es:di + 20], dword 1   ; force a valid ACPI 3.x entry
    mov ecx, 24
    int 0x15
    jc .end_e820

    mov edx, 0x534D4150         ; some bios will throw the value of edx
                                ; so we reset it to be sure that it is still set to "SMAP"
.check_output:
    jcxz .skip_entry            ; if the entry is equal to 0 skip the entry and go the the next one
    cmp cl, 20                  ; Check if the BIOS returned a ACPI 3.x 
                                ; CX contain the number of byte returned, so if cl is smaller than 20
                                ; the response is not an ACPI 3.x format.
    jbe .no_text                ; if response is not an ACPI 3.x format response then jump to no_text
                                ; most BIOS will only write 20 bytes so no extended attribute
    test byte [es:di + 20], 1   ; check if the  Extended Attributes bitfield bit is set
                                ; if Bit 0 of the Extended Attributes indicates if the entire entry should be ignored
                                ; if Bit 1 of the Extended Attributes indicates if the entry is non-volatile 
    je .skip_entry              ; if Bit 1 then skip the entry

.no_text:
    mov ecx, [es:di + 8]        ; get the lower uint32 of memory region length
    or ecx, [es:di + 12]        ; use or to check if region length is 0
    jz .skip_entry
    inc bp                      ; good entry so increase bp
    add di, 24                  ; set di to the ext address

.skip_entry:
    test ebx, ebx               ; check if ebx == 0
    jne .e820_loop              ; if ebx != 0 jmp to e820_loop

.end_e820:
    mov [MEMORY_ENTRY_COUNT], bp; set the number of entry into ENTRY_COUNT
    clc                         ; clear carry flag
    jmp .exit

.failed:
    stc                         ; Set carry flag to 1.

.exit:
    popa
    ret

MEMORY_ENTRY_COUNT:
    dw 0x0000       ;

MEMORY_SIZE_KB:
    dd 0x00000000   ; Base address uint64 lower 4 bytes
    dd 0x00000000   ; Base address uint64 higher 4 bytes
    dd 0x00000000   ; Length uint64 lower 4 bytes
    dd 0x00000000   ; Length uint64 higher 4 bytes
    dd 0x00000000   ; entry type
    dd 0x00000000   ; extended
; the unreal mode keeps real mode addressing while the segment descriptors
; carry a 4GiB limit.
; entering protected mode fills the descriptor cache of a segment register
; from the gdt. leaving protected mode does not clear that cache : addressing
; goes back to segment * 16 + offset, but the limit stays at whatever the
; descriptor said it was.
; so ds and es can reach 4GiB while int 0x13 still works, which is exactly
; what we need : the bios to read the disk, and the whole address space to
; copy into.
; ex: mov esi, 0x10000000 becomes a legal read.
; in plain real mode a segment can only see 64KiB from its base, so that
; same read would wrap inside the segment instead.
;
; interrupts must be off for the whole thing : an irq taken with a protected
; mode cs and a real mode idt goes nowhere good.

_EnterUnrealMode:
    cli                         ; disable interrupt
    push ds                     ; save the segments. we only want their descriptor
    push es                     ; cache changed, not the values they hold.

    lgdt [UNREAL_GDT.Pointer]   ; load our temporary 32bit gdt

    mov eax, cr0                ; set eax to cr0
    or al, 1                    ; set the pe-bit
                                ; pe-bit enable protected mode
                                ;   - to 0 = real mode
                                ;   - to 1 = protected mode
    mov cr0, eax                ; set the new value to cr0

    jmp $+2                     ; flush the prefetch queue

    mov bx, 0x08                ; the 4GiB data descriptor, .Data of UNREAL_GDT.
                                ; a literal and not the label : .Data is an equ
                                ; on $, and reading it before the table is laid
                                ; out stops nasm from converging.
    mov ds, bx                  ; loading it is what fills the descriptor cache of
    mov es, bx                  ; ds and es with the 4GiB limit

    mov eax, cr0                ; set eax to cr0
    and al, 0xfe                ; clear the pe-bit, so back to real mode
    mov cr0, eax                ; set the new value to cr0

    pop es                      ; recover the real mode segment values.
    pop ds                      ; the cache keeps the 4GiB limit though, and that
                                ; is the undocumented part the whole trick rests on.
    sti                         ; enable interrupt
    ret

; a temporary 32bit gdt, not the GDT64 of gdt.asm. it only lives long enough
; to fill a descriptor cache, and is never loaded again.
UNREAL_GDT:
    .Null: equ $ - UNREAL_GDT       ; The null descriptor.
    dw 0                            ; Limit (low).
    dw 0                            ; Base (low).
    db 0                            ; Base (middle)
    db 0                            ; Access.
    db 0                            ; Granularity.
    db 0                            ; Base (high).
    .Data: equ $ - UNREAL_GDT       ; The data descriptor.
    dw 0xffff                       ; Limit (low).
    dw 0                            ; Base (low).
    db 0                            ; Base (middle)
    db 10010010b                    ; Access (read/write).
    db 11001111b                    ; Granularity 4KiB, 32 bits flag, limit19:16.
    db 0                            ; Base (high).
    .Pointer:                       ; The GDT-pointer.
    dw $ - UNREAL_GDT - 1           ; Limit.
    dd UNREAL_GDT                   ; Base.

; Global Descriptor Table 
; the GDT is a table made with segment descriptor
; A segment descriptor describe a segment


GDT64:                              ; Global Descriptor Table (64-bit).
    .Null: equ $ - GDT64            ; The null descriptor.
    dw 0                            ; Limit (low).
    dw 0                            ; Base (low).
    db 0                            ; Base (middle)
    db 0                            ; Access.
    db 0                            ; Granularity.
    db 0                            ; Base (high).
    .Code: equ $ - GDT64            ; The code descriptor.
                                    ; this will contain the code
    dw 0                            ; Limit (low).
    dw 0                            ; Base (low).
    db 0                            ; Base (middle)
    db 10011000b                    ; Access (exec/read).
    db 00100000b                    ; Granularity, 64 bits flag, limit19:16.
    db 0                            ; Base (high).
    .Data: equ $ - GDT64            ; The data descriptor.
    dw 0                            ; Limit (low).
    dw 0                            ; Base (low).
    db 0                            ; Base (middle)
    db 10000000b                    ; Access (read/write).
    db 00000000b                    ; Granularity.
    db 0                            ; Base (high).
    .Pointer:                       ; The GDT-pointer.
    dw $ - GDT64 - 1                ; Limit.
    dq GDT64                        ; Base.
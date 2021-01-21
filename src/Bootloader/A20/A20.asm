; the A20 line is an option that if enabled
; won't allow address bigger than 20bit
; so if an address that is 21bit long is passed
; it will remove that 21st bit.
; ex:bx ex = 0x1000 bx = 0x7c00
; (0x1000 << 4) + 0x7c00
; 0x10000 + 0x7c00
; 0x17c00
; This is how 20bits address are accessed.
; the ":" in ex:bx is basically doing (ex * 16) + bx
; but if the address that result is larger than 20bits
; with the A20 line disable it will return at the address 0x0000
; and start offsetting from there.
; 0xf800:0x8000 = (0xf800 << 4) + 0x8000 = 0x100000
; with the A20 line disable the address will stay 0x100000
; with the A20 line enable the address will be trunc to 0x00000 so 0x0000

_TestA20:
    pusha                       ; save the register into the stack

    mov ax, [0x7dfe]            ; put the magic number into ax

    xor bx, bx                  ; clear bx
    not bx                      ; bx ~= bx set bx to 0xffff
    mov es, bx                  ; put bx into es

    mov bx, 0x7e0e              ; set bx to 0x7e0e
    mov dx, [es:bx]             ; check the value of 0xffff0:0x7e0e ( 0x107DFE )

    cmp ax, dx                  ; if the value of 0x107dfe is equal to 0x7dfe
    je .A20Disable              ; the A20 line is disable.

    mov ax, [0x7dff]            ; do a second test with all the address shift by 1 byte

    xor bx, bx                  ; clear bx
    not bx                      ; bx ~= bx set bx to 0xffff
    mov es, bx                  ; put bx into es

    mov bx, 0x7e0f              ; set bx to 0x7e0e (so 0x7e0e + 0x0001)
    mov dx, [es:bx]             ; put the value into dx

    cmp ax, dx                  ; check if ax is equal to dx
    je .A20Disable              ; if equal the A20 line is definitively disable

    mov ax, 1                   ; clear ax 

    jmp .exit                   ; go to exit
.A20Disable:
    mov ax, 0                   ; set ax to 1 for return value
    call _EnableA20Line
    
.exit:
    popa                        ; recover the registers from the stack
    ret 

_EnableA20Line:
    pusha
    in al, 0x92         ; read data through port 0x92
                        ; get the data from the chipset
    or al, 2            ; mask bit 2
                        ; the bit 2 is the one that toggle A20 line
    out 0x92, al        ; send data back to chipset

    call _TestA20

    cmp ax, 0
    jne .exit

    PrintStringNextLine A20_LINE_ERROR

    jmp $               ; infinite loop the OS can't start without the A20 line enable!

.exit:
    popa
    ret

A20_LINE_ERROR:
    db "Er A20",0
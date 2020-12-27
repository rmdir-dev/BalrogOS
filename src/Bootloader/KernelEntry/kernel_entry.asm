[bits 64]                       ; switching to 64bit
[extern main]

section .text
_KernelEntry:
    call main
    hlt                         ; halt
    sti                         ; enable interrupt
    ret

    global addTwo
addTwo:
    mov eax, edi
    add eax, esi
    ret

MSG:
    db 'PRINT THIS!',0


%include "src/Bootloader/IO/System/print.asm"
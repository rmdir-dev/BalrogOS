[bits 64]                       ; switching to 64bit
[extern main] 

_KernelEntry:
    call main
    hlt                         ; halt
    sti                         ; enable interrupt
    ret

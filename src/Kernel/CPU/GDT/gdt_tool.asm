section .text
    global flush_gdt
    global flush_tss

flush_gdt:
    lgdt [rdi]
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov rax, .flush
    push 0x08
    push rax
    ;jmp $
    retfq
.flush:
    ret

flush_tss:
    mov ax, 0x2b
    ltr ax
    ret
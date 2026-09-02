section .text
    global flush_gdt
    global flush_tss

flush_gdt:
    lgdt [rdi]          ; load the new GDT pointer
    mov ax, 0x10        ; mox 0x10 (SEG KERNEL DATA) into ax
    mov ds, ax          ; set data segments do ax
    mov es, ax          ; set data segments do ax
    mov fs, ax          ; set data segments do ax
    mov gs, ax          ; set data segments do ax
    mov ss, ax          ; set data segments do ax
    mov rax, .flush     ; put .flush address into rax
    push 0x08           ; put 0x08 (SEG KERNEL CODE) into the stack
    push rax            ; push rax
    retfq               ; long jump to check if the new GDT work
                        ; jmp 0x08:.flush
.flush:
    ret                 ; return

flush_tss:
    mov ax, 0x2b        ; put the index of the TSS + 3 into ax
    ltr ax              ; load the new TSS
    ret
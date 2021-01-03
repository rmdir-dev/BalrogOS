section .text
    global _set_idt
    global _load_idt
    global isr_common
    extern kernel_interrupt_handler

_preset_idt:
    ret

_load_idt:
    lidt [rdi]
    ret

isr_common:
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    ; clear RFLAGS
    push qword 0
    popf

    call kernel_interrupt_handler

    iret    ; interrupt return
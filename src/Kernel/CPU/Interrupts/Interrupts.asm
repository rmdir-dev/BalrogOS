section .text
    global _set_idt
    global _load_idt
    global isr_common
    global _isr_return
    extern kernel_interrupt_handler
    extern user_mode_print

_preset_idt:
    ret

_load_idt:
    lidt [rdi]
    ret

isr_common:
    push r15    ; push all the registers to the stack
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

    ; check if the RFLAGS RING 3 is set (user mode)
    mov rax, [rsp + 152]
    and rax, 3 << 12
    jz .kernel_mode     ; if in usermode then swapgs
    swapgs

.kernel_mode:
    mov rdi, rsp
    call kernel_interrupt_handler
    mov rdi, rax

; ISR will be use for context switch
; We will use it to jump directly here from the new stack at first exec
_isr_return:
    mov rsp, rdi

    ; check if we're comming from user mode
    mov rax, [rsp + 152]
    and rax, 3 << 12
    jz .kernel_return
    swapgs

.kernel_return:
    pop rax     ; recover all the register state
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    add rsp, 0x10   ; add 0x10 (16 bytes) to the stack
                    ; we've push the interrupt nbr and error message
                    ; both are 8bytes so we add rsp 16 to avoid poping
                    ; these into registers
    ;jmp $

    iretq    ; interrupt return
    
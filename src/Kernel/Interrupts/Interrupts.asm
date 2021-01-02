section .text
    global _set_idt
    global _load_idt
    global isr_common

_set_idt:
    ret

_load_idt:
    lidt [rdi]
    ret

isr_common:
    ret
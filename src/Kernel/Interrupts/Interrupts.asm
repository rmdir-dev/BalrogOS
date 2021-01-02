section .text
    global _set_idt
    global _load_idt

_set_idt:
    ret

_load_idt:
    lidt [rdi]
    ret
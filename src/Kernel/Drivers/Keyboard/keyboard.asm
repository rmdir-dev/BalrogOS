section .text
    global _EnableKeyboard

_EnableKeyboard:    
    mov al, 0xfd        ; enable IRQ 0x20
    out 0x21, al

    ret
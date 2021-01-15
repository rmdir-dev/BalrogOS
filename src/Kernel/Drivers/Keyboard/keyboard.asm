section .text
    global _EnableKeyboard

_EnableKeyboard:
    ;in al, 0x21        ; enable IRQ 0x21 and 0x20
    ;and al, 0xfc
    ;out 0x21, al
    
    mov al, 0xfd        ; enable IRQ 0x20
    out 0x21, al

    ret
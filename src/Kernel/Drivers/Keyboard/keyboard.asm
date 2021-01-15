section .text
    global _EnableKeyboard

_EnableKeyboard:
    ;in al, 0x21
    ;and al, 0xfc
    ;out 0x21, al
    
    mov al, 0xfd
    out 0x21, al

    ret
section .text
    global _EnableKeyboard

_EnableKeyboard:
    in al, 0x21         ; recover PIC Master mask

    and al, 0xfd        ; enable IRQ 0x20
                        ; 0b1111 1101
                        ; Interrupt mask
                        ; Here I enable IRQ_1 (keybord) which is on the second byte
                        ; so the mask is 0b1111 1101 to enable the second byte
                        ; If I would have to enable the IRQ_0 I would need to use this mask
                        ; 0b1111 1100
                        ; This would enable IRQ_1 and IRQ_0

    out 0x21, al        ; Now we set the 0x21 oort to al
                        ; 0x21 is the Master PIC mask
                        ; 0xa1 is the slave PIC mask
    ret
; INPUT dx as an hex value
_PrintHex:
    push cx
    push di
    push bx

    mov si, HEX_OUT             ; set si to HEX_OUT si will be use for _PrintString
    mov cl, 12                  ; set 12 into cl for the first shifting
    mov di, 2                   ; Set the counter to 2
                                ; HEX_OUT is '0xXXXX' and we don't want to override the two first bytes

.hexToCharLoop:
    mov bx, dx                  ; put the value in dx into bx, to avoid using the original passed value
    shr bx, cl                  ; shift right bl of value of cl bytes
    and bx, 0x000f              ; mask the unwanted values
    mov bx, [bx + HEX_TABLE]    ; mov the correct char into bx
    mov [HEX_OUT + di], bl      ; mov the char in bx to HEX_OUT >> di
    sub cl, 4                   ; sub 4 to cl to access the next hex to convert
    inc di                      ; increment di to write to the next HEX_OUT value.

    cmp di, 6                   ; check if di is equal to 6, if yes then exit
    je .exit

    jmp .hexToCharLoop

.exit:
    call _PrintString

    pop bx                      ; recover the saved values
    pop di                      ; recover the saved values
    pop cx                      ; recover the saved values
    ret

HEX_OUT:
    db '0x0000',0
HEX_TABLE:
    db '0123456789abcdef'


; INPUT : si = address of the string
_PrintString:
    pusha                   ; push everything !! only possible in 16bit mode
                            ; pushad in 32bit
    mov ah, 0x0e            ; set ah to 0x0e to say that we want to display a charater
                            ; is combined with int 0x10
                            ; https://wiki.osdev.org/BIOS

.printLoop:
    mov al, [si]            ; set the content of si to al
    cmp al, 0               ; compare al to 0
    jne .printChar          ; jump to .printChar if al is not equal to 0
    popa                    ; pop everything from the stack !!! only possible in 16bit mode
                            ; popad in 32bit
    ret                     ; return

.printChar:
    int 0x10                ; execute the interrupt 0x10
                            ; 0x10 use ax as parameter
                            ; ah to 0x0e mean display char
                            ; al is the char to print
    inc si                  ; increment si to go to the next character
    jmp .printLoop

RETURN_TO_LINE:
    db 0x0a,0x0d,0          ; 0x0a = return to line
                            ; 0x0d = reset the cursor to the begining of the line
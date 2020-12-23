%define VIDEO_MEMORY 0xb8000
%define WHITE_CHAR  0x0f

; INPUT : si = address of the string
_SysPrintString:
    ;pushad                  ; push everything !! only possible in 16bit mode
                            ; pushad in 32bit
    mov edx, [VIDEO_MEMORY]; set edx at the start of video memory

.printLoop:
    mov al, [rsi]            ; set the content of si to al
    mov ah, WHITE_CHAR      ; set a white char on black background

    cmp al, 0               ; compare al to 0
    jne .printChar          ; jump to .printChar if al is not equal to 0

    ;popad                   ; pop everything from the stack !!! only possible in 16bit mode
                            ; popad in 32bit
    ret                     ; return

.printChar:
    mov [edx], ax

    add edx, 2
    inc si                  ; increment si to go to the next character
    jmp .printLoop
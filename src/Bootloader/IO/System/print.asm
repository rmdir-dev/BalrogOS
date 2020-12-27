%define VIDEO_MEMORY 0xb8000
%define WHITE_CHAR  0x0f

; INPUT : si = address of the string
_SysPrintString:
    mov edi,  VIDEO_MEMORY  ; set edx at the start of video memory
    mov ah, WHITE_CHAR      ; set a white char on black background

.printLoop:
    mov al, [esi]
    cmp al, 0
    je .endPrintLoop

    call _printChar
    inc esi
    jmp .printLoop

.endPrintLoop:
    ret

_printChar:
    mov [edi], eax
    add edi, 2
    ret
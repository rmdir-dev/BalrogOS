%macro PrintStringNextLine 1
    mov si, %1                  ; set the address of the string into the si register
    call _PrintString
    mov si, RETURN_TO_LINE      ; set the return to line to the si register
    call _PrintString
%endmacro

%macro PrintString 1
    mov si, %1                  ; set the address of the string into the si register
    call _PrintString
%endmacro

%macro PrintHexToLine 1
    mov dx, %1                  ; put the hex value into the dx register
    call _PrintHex
    mov si, RETURN_TO_LINE      ; set the return to line to the si register
    call _PrintString
%endmacro

%macro PrintHex 1
    mov dx, %1                  ; put the hex value into the dx register
    call _PrintHex
%endmacro
[bits 16]                       ; switching to 64bit
[extern kernel_main]

%define KERNEL_OFFSET 0xFFFFFF8000000000

section .text
    global _PrepareKernel
_PrepareKernel:
    cli                     ; disable interrupt

    mov [MEMORY_INFO - KERNEL_OFFSET], word ax ; recover memory map
    mov [MEMORY_ENTRIES - KERNEL_OFFSET], word bx
    
    mov edi, 0x1000 - KERNEL_OFFSET        ; set the destination index to 0x1000
    mov cr3, edi            ; set control register 3 to destination
    xor eax, eax            ; nullify eax
    mov ecx, 4096           ; set ecx to 4096 (will be use as a counter)
    rep stosd               ; clear the memory from 0x1000 to 0x5000
    mov edi, 0x1000 - KERNEL_OFFSET         ; set edi back to 0x1000 (PML4T)

    ; PAGING                                                    total paging cover 256TiB of memory
    ; PML4T address 0x1000 pointing to PDPT                     each PML4T hold 512GiB
    ; PDPT  address 0x2000 pointing to PDT                      each PDPT hold 1GiB
    ; PDT   address 0x3000 pointing to PT                       each PDT hold 2MiB
    ; PT    address 0x4000 pointing to the pages                each PT hold 4kiB (pages)

    mov dword [edi], 0x2003 - KERNEL_OFFSET     ; Set the addres of the begining of PDPT to the first address of PML4T
    mov edi, 0x2000 - 8 - KERNEL_OFFSET
    mov dword [edi], 0x2003 - KERNEL_OFFSET     ; set the last PML4T for higher half kernel
    mov edi, 0x1000 - KERNEL_OFFSET             ; set edi back to 0x1000 (PML4T)
                                ; the two first bytes are the pointer to the next table
                                ; 2003 3 is for 0b11 = present and writable
    add edi, 0x1000             ; add 0x1000 to edi so now edi point to 0x2000
    mov dword [edi], 0x3003 - KERNEL_OFFSET     ; Set the addres of the begining of PDPT to PDT
    add edi, 0x1000             ; add 0x1000 to edi so now edi point to 0x3000
    mov dword [edi], 0x4003 - KERNEL_OFFSET     ; Set the addres of the begining of PDT to PT
    add edi, 0x1000             ; add 0x1000 to edi so now edi point to 0x4000
    

    mov dword ebx, 0x00000003   ; ebx to 0x00000003 3 = present and writable
    mov ecx, 512                ; ecx to 512 will be use as counter
    
.SetEntry:
    mov dword [edi], ebx        ; set ebx into edi
    add ebx, 0x1000             ; add 0x1000 to ebx
    add edi, 8                  ; add 8 to edi (shift 8 byte so to the next address)
    loop .SetEntry              ; loop while ecx is not equal to 0
                                ; the map the first 2MB of memory
                                ; so it will map the memory from 0x00000000 to 0x00200000

    mov eax, cr4                ; set the cr4 register to eax
    or eax, 1 << 5              ; set the PAE-bit to 1
                                ; physical address extension
                                ; If set, changes page table layout to translate 32-bit virtual addresses 
                                ; into extended 36-bit physical addresses.
                                ; enable the PAE paging
    mov cr4, eax                ; set the new value to cr4

    mov ecx, 0xC0000080         ; set ecx to EFER MSR
    rdmsr                       ; read from the model specific register
    or eax, 1 << 8              ; set the LM-bit to 1 (long mode = x64)
    wrmsr                       ; write to msr

    mov eax, cr0                ; set eax to cr0
    or eax, 1 << 31 | 1 << 0    ; set the pg-bit and the pe-bit
                                ; pg-bit enable paging
                                ;   - to 0 = paging disable
                                ;   - to 1 = paging enable and use CR3 register
                                ; pe-bit enable protected mode
                                ;   - to 0 = real mode
                                ;   - to 1 = protected mode
    mov cr0, eax                ; set the new value to cr0

    lgdt [GDT64.Pointer - KERNEL_OFFSET]        ; load gdt

    jmp GDT64.Code:(LongMode - KERNEL_OFFSET)

%include "src/Bootloader/LongMode_x64/gdt.asm"

[bits 64]                       ; switching to 64bit

LongMode:
    mov eax, 0x0                ; clearing segment register
    mov ss, eax
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax

    mov rax, qword Upper_half   ; Upper_half address into rax
    jmp rax

Upper_half:
    mov rax, KERNEL_OFFSET      ; set rsp to higher half
    add rsp, rax                ; set rsp to higher half

    mov rax, 0                  ; disable the lower 2MB pages
    mov [0x1000 - KERNEL_OFFSET], qword rax

    mov rax, cr3                ; update the cr3 register
    mov cr3, rax                ; to update paging informations

    mov rax, qword GDT64.Pointer ; update GDT
    lgdt [rax]

    
    mov rax, 0                  ; clear the segment registers
    mov ss, rax
    mov ds, rax
    mov es, rax

    mov rax, qword .reload_cs   ; ensure that we're in higher half
    push qword 0x8
    push rax
    retfq
.reload_cs:

    mov rax, qword _KernelEntry ; call to kernel entry
    call rax
    hlt                         ; halt
    sti                         ; enable interrupt
    ret

;%include "src/Bootloader/IO/System/print.asm"

IN_x64_PROTECTED_MODE_MSG:
    db "[LOAD] x64 protected mode",0

MEMORY_INFO:
    dw 0x0000

MEMORY_ENTRIES:
    dw 0x0000

[bits 64]
section .text
_KernelEntry:
    mov rax, qword MEMORY_INFO
    mov word di, [rax]
    mov rax, qword MEMORY_ENTRIES
    mov word si, [rax]
    mov rax, qword kernel_main  ; call main
    call rax
    jmp $
    sti                         ; enable interrupt
    hlt                         ; halt
    ret

    global addTwo
addTwo:
    mov eax, edi
    add eax, esi
    ret

%include "src/Bootloader/IO/System/print.asm"
[bits 16]                       ; switching to 64bit

%define KERNEL_OFFSET 0xFFFFFFFF80000000

section .text
    global _PrepareKernel
_PrepareKernel:
    cli                     ; disable interrupt

    mov [MEMORY_INFO - KERNEL_OFFSET], word ax ; recover memory map
    mov [MEMORY_ENTRIES - KERNEL_OFFSET], word bx
    
    mov edi, 0x1000        ; set the destination index to 0x1000
    mov cr3, edi            ; set control register 3 to destination
    xor eax, eax            ; nullify eax
    mov ecx, 5120           ; set ecx to 5120 (will be use as a counter)
    rep stosd               ; clear the memory from 0x1000 to 0x6000 only !
                            ; the e820 count lives at 0x6FFE, the text pdt
                            ; clears itself below.
    mov edi, 0x1000         ; set edi back to 0x1000 (PML4T)
    
    ; PAGING                                                    total paging cover 256TiB of memory
    ; PML4T address 0x1000 pointing to PDPT                     each PML4T hold 512GiB / entry total 256TiB
    ; PDPT  address 0x2000 pointing to PDT                      each PDPT hold 1GiB / entry total 512 GiB
    ; PDT   address 0x3000 pointing to PT                       each PDT hold 2MiB / entry total 1024MiB
    ; PT    address 0x4000 pointing to the pages                each PT hold 4kiB / entry total 2MiB

    mov dword [edi], 0x2003     ; Set the addres of the begining 
                                                ; of PDPT to the first address of PML4T
    mov edi, 0x2000 - 8
    mov dword [edi], 0x2103     ; set the last PML4T for higher half kernel
    mov edi, 0x1000             ; set edi back to 0x1000 (PML4T)
                                ; the two first bytes are the pointer to the next table
                                ; 2103 3 is for 0b11 = present and writable
                                ; 100 is for page global
    add edi, 0x1000             ; add 0x1000 to edi so now edi point to 0x2000
    mov dword [edi], 0x3003     ; Set the addres of the begining of PDPT to PDT
    add edi, 0x1000             ; add 0x1000 to edi so now edi point to 0x3000
    mov dword [edi], 0x4003     ; Set the addres of the begining of PDT to PT
    add edi, 0x1000             ; add 0x1000 to edi so now edi point to 0x4000
    

    mov dword ebx, 0x00000103   ; 0x3 = present and writable, 0x100 is the global bit.
                                ; It is ignored while CR4.PGE is 0, and PGE only goes
                                ; up in Upper_half, once the identity map is gone.
    mov ecx, 512                ; ecx to 512 will be use as counter
    
.SetEntry:
    mov dword [edi], ebx        ; set ebx into edi
    add ebx, 0x1000             ; add 0x1000 to ebx
    add edi, 8                  ; add 8 to edi (shift 8 byte so to the next address)
    loop .SetEntry              ; loop while ecx is not equal to 0
    
                                ; the map the first 2MB of memory
                                ; so it will map the memory from 0x00000000 to 0x00200000

    ; 2MiB up to 8MiB, as three 2MiB pages instead of three more tables.
    ; the early kernel reaches the pages the pmm hands out through P2V, and
    ; that happens before init_vmm has built anything of its own, so the
    ; window has to be wider than the image.
    ; bit 7 is PS, which makes the entry a 2MiB page rather than a pointer.
    mov edi, 0x3000 + 8 ; pdt entry 1, the first one past the pt
    mov dword ebx, 0x00200183   ; 0x200000, present, writable, global, and PS
    mov ecx, 3                  ; three entries, so up to 0x800000

.SetHugeEntry:
    mov dword [edi], ebx        ; the high half stays zero, the rep stosd above
                                ; cleared 0x1000 to 0x5000 before we got here
    add ebx, 0x200000           ; walk one 2MiB page forward
    add edi, 8                  ; and one entry forward
    loop .SetHugeEntry

    ; KERNEL_TEXT_BASE is 0xFFFFFFFF80000000, pdpt entry 510 of the same
    ; PML4T[511] slot. bootx64.c builds the very same window, keep them equal.
    mov edi, 0x2000 + 510 * 8   ; pdpt entry 510
    mov dword [edi], 0x6003     ; the text pdt at 0x6000

    ; the rep stosd above stopped at 0x6000, so this one clears itself
    mov edi, 0x6000             ; the text pdt
    xor eax, eax                ; nullify eax
    mov ecx, 1024               ; 512 entries of 8 bytes
    rep stosd

    ; four 2MiB pages cover the first 8MiB, where the image sits
    mov edi, 0x6000 ; the text pdt
    mov dword ebx, 0x00000183   ; 0x0, present, writable, global, and PS
    mov ecx, 4                  ; four entries, so up to 0x800000

.SetTextEntry:
    mov dword [edi], ebx        ; the high half stays zero, the rep stosd just
                                ; above cleared the whole table
    add ebx, 0x200000           ; walk one 2MiB page forward
    add edi, 8                  ; and one entry forward
    loop .SetTextEntry

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

%include "src/bootloader/common/long_mode_x64/gdt.asm"

[bits 64]                       ; switching to 64bit
[extern kernel_main]
[extern bss_start]
[extern bss_end]

LongMode:
    mov eax, 0x0                ; clearing segment register
    mov ss, eax
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax

    mov rax, qword Upper_half   ; Upper_half address into rax
                                ; qword because it needs the virtual address of Upper_half
    jmp rax

Upper_half:
    mov rax, KERNEL_OFFSET      ; set rsp to higher half
    add rsp, rax                ; set rsp to higher half
    
    mov rax, 0                  ; disable the lower 2MB pages
    mov [0x1000], qword rax

    mov rax, cr3                ; update the cr3 register
    mov cr3, rax                ; to update paging informations

    ; the kernel lives in PML4T[511] and every process points at the same pdpt,
    ; so its translations are identical in every address space. PGE tells the cpu
    ; that much -> a mov to cr3 stops flushing them and the kernel keeps its tlb
    ; across a context switch.
    ;
    ; it belongs here and not next to PAE above ! while the identity map at
    ; PML4T[0] was up, the low 2MiB went through the very same page tables as the
    ; higher half. marking them global before dropping that entry would have left
    ; the identity map alive in the tlb, where no cr3 reload can reach it.
    ; source : intel sdm vol 3A, 4.10.2.4 global pages
    mov rax, cr4
    or rax, 1 << 7              ; PGE
    mov cr4, rax

    mov rax, qword GDT64.Pointer ; update GDT
    lgdt [rax]

    
    mov rax, 0                  ; clear the segment registers
    mov ss, rax
    mov ds, rax
    mov es, rax

    mov rax, qword .reload_cs   ; ensure that we're in higher half
    push 0x8              ; by reloading the code segment
    push rax                    ; 0x8 = code segment index in the GDT
    retfq                       ; does a long jump CODE:.reload_cs
.reload_cs:
    mov rax, qword _KernelEntry ; call to kernel entry
    call _KernelEntry
    hlt                         ; halt
    sti                         ; enable interrupt
    ret
_KernelEntry:
;   the .bss is a NOBITS section : it takes no room in kernel.bin and neither
;   loader writes it, so every static that is supposed to start at zero starts
;   on whatever was left in that ram. qemu hands out a machine full of zeroes
;   and hides the whole thing, a real one does not. idt_ptr, int_handlers and
;   the rest come up holding garbage.
;   here is the place to do it : nothing has run yet, and the kernel stack sits
;   in there too so there is nothing to lose by clearing it.
    cld                         ; rep goes forward, we do not inherit a df
    mov rdi, qword bss_start
    mov rcx, qword bss_end
    sub rcx, rdi                ; how many bytes the section covers
    xor eax, eax
    rep stosb

    mov rax, qword MEMORY_INFO
    movzx rdi, word [rax]       ; movzx and not a 16 bit mov : the bootloader
    mov rax, qword MEMORY_ENTRIES
    movzx rsi, word [rax]       ; leaves the upper halves of rsi and rdi dirty,
                                ; _LoadRamfs copies with esi and edi. a 16 bit
                                ; mov only replaces the low word, and the map
                                ; would be read 0x90000 too high.

    mov rsp, kernel_stack_top   ; move rsp to kernel stack top

    call kernel_main
    jmp $
    sti                         ; enable interrupt
    hlt                         ; halt
    ret

IN_x64_PROTECTED_MODE_MSG:
    db "[LOAD] x64 protected mode",0
    
MEMORY_INFO:
    dw 0x0000

MEMORY_ENTRIES:
    dw 0x0000

section .bss
;   64KiB of stack for the kernel.
align 16
kernel_stack_bottom:
    resb 65536
kernel_stack_top:

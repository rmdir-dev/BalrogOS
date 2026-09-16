; the disk layout, shared by the two stages and mirrored in the makefile.
;
;   lba 0           the mbr : stage 1, the partition table at offset 446, 0xaa55
;   lba 1-2047      the mbr gap, where stage 2 goes
;   lba 2048        the first partition, the kernel then the ramfs, raw
;
; a makefile can't include a nasm file, so the makefile carries the same
; numbers. this file is where they are decided.

; -----------------------------------------------------------------------------
; on the disk, counted in sectors of 512 bytes
; -----------------------------------------------------------------------------

STAGE2_LBA      equ 1           ; the sector right behind the boot sector
STAGE2_SECTORS  equ 16          ; 16 * 512B = 8KiB
                                ; _DiskLoad takes the count in dh, a byte, so
                                ; this has to stay under 256.

PART1_LBA       equ 2048        ; the first partition, 1MiB into the disk

KERNEL_LBA      equ PART1_LBA   ; the kernel sits at the head of the partition
KERNEL_SECTORS  equ 768         ; 768 * 512B = 384KiB
                                ; kernel.bin is 558 sectors since we added lai.
                                ; it has to be a multiple of CHUNK_SECTORS, that
                                ; is the step _LoadKernel reads with.
                                ; the kernel ends at 0x58000, so far below the
                                ; buffer at 0x90000.

RAMFS_LBA       equ KERNEL_LBA + KERNEL_SECTORS; right behind the kernel
RAMFS_SECTORS   equ 16384       ; 16384 * 512B = 8MiB
                                ; _DiskLoad puts the lba in di, 16 bits, so the
                                ; last sector it can reach is 65535.

PART1_SECTORS   equ KERNEL_SECTORS + RAMFS_SECTORS; what the mbr entry declares

; -----------------------------------------------------------------------------
; in memory
; -----------------------------------------------------------------------------

STAGE2_ADDR     equ 0x0500      ; the first free byte above the bios data area,
                                ; and about 30KiB up to the boot sector at 0x7c00
KERNEL_ADDR     equ 0x8000      ; same value as KERNEL_START in Kernel.ld
RAMFS_PHYS      equ 0x10000000  ; 256MiB. the bios only writes below 1MiB, so the
                                ; ramfs is read into LOAD_BUFFER_SEG and copied up
                                ; here in unreal mode, one chunk at a time.

LOAD_BUFFER_SEG equ 0x9000      ; 0x90000, the low buffer each chunk lands in.
                                ; above the kernel, below the ebda, and 64KiB
                                ; aligned so _DiskLoad never splits on the dma
                                ; boundary.
CHUNK_SECTORS   equ 64          ; 64 * 512B = 32KiB, one copy up.
                                ; under 256 for dh, and it fits in the buffer.

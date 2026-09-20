ENTRY(_PrepareKernel)

KERNEL_TEXT_BASE = 0xFFFFFFFF80000000;
KERNEL_START = 0x8000;

SECTIONS
{
  . = KERNEL_START + KERNEL_TEXT_BASE;
  kernel_start = .;
  .text : AT(ADDR(.text) - KERNEL_TEXT_BASE)
  {
    *(.multiboot)
    *(.text)
  }
  .rodata : AT(ADDR(.rodata) - KERNEL_TEXT_BASE)
  {
    *(.rodata*)
    *(.lrodata*)
  }
  .data : AT(ADDR(.data) - KERNEL_TEXT_BASE)
  {
    *(.data)
    *(.ldata*)
  }
  .bss (NOLOAD) : AT(ADDR(.bss) - KERNEL_TEXT_BASE)
  {
    bss_start = .;
    *(.COMMON)
    *(.bss)
    *(.lbss)
    bss_end = .;
  }
  kernel_end = .;

  /*
  kheap in NOLOAD, pmm_reserve this area during init.
  */
  . = 0x100000 + KERNEL_TEXT_BASE;
  .kheap (NOLOAD) : AT(ADDR(.kheap) - KERNEL_TEXT_BASE)
  {
    kheap_area = .;
    /* 1MiB Kheap */
    . = . + 1024K;
    kheap_area_end = .;

    /* 4KiB buffer */
    . = . + 4K;
    kheap_guard_end = .;
  }

  .screen (NOLOAD) : AT(ADDR(.screen) - KERNEL_TEXT_BASE)
  {
    screen_buffers = .;
    /* FB_BUFFERS_SIZE, and fb_init checks it fits */
    . = . + 256K;
    screen_buffers_end = .;
  }

  /* kernel  logs space */
  .klog (NOLOAD) : AT(ADDR(.klog) - KERNEL_TEXT_BASE) ALIGN(0x10000)
  {
    klog_area = .;
    /* 1MiB Kheap */
    . = . + 4096K;
    klog_area_end = .;

    /* 4KiB buffer */
    . = . + 4K;
    klog_guard_end = .;
  }
}

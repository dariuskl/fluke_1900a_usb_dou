OUTPUT_ARCH(msp430)
ENTRY(on_reset)

MEMORY {
    ram (rw) : ORIGIN = 0x0200, LENGTH = 256
    info (r) : ORIGIN = 0x1000, LENGTH = 256
    rom (rx) : ORIGIN = 0xe000, LENGTH = 8192 - 64
    vectors : ORIGIN = 0xffc0, LENGTH = 64
}

SECTIONS {
  .vectors :
  {
    KEEP (*(.vectors))
  } > vectors

  .rodata :
  {
    . = ALIGN(2);
    _preinit_array_start = .;
    KEEP(*(.preinit_array))
    _preinit_array_end = .;
    . = ALIGN(2);
    _init_array_start = .;
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    _init_array_end = .;
    . = ALIGN(2);
    _fini_array_start = .;
    KEEP(*(.fini_array))
    KEEP(*(SORT(.fini_array.*)))
    _fini_array_end = .;
    . = ALIGN(2);
    *(.rodata .rodata.*)
  } > rom

  .text :
  {
    . = ALIGN(2);
    *(.text)
  } > rom

  .data :
  {
    . = ALIGN(2);
    _sdata = .;
    *(.data .data.* .gnu.linkonce.d.*)
    . = ALIGN(2);
    _edata = .;
  } > ram AT>rom

  PROVIDE(_sidata = LOADADDR(.data));

  .bss :
  {
    . = ALIGN(2);
    _sbss = .;
    *(.bss)
    . = ALIGN(2);
    _ebss = .;
  } > ram

  .stack (ORIGIN(ram) + LENGTH(ram)) :
  {
    _stack = .;
    *(.stack)
  }

}

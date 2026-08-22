ENTRY(reset_handler)

_estack = ORIGIN(RAM) + LENGTH(RAM);

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN =  0x20000000, LENGTH = 128K
}

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
    } >FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text) *(.text*)
        *(.rodata) *(.rodata*)
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP(*(.preinit_array))
        __preinit_array_end = .;
        . = ALIGN(4);
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
        . = ALIGN(4);
    } >FLASH

    _siData = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sData = .;
        *(.data*)
        . = ALIGN(4);
        _eData = .;
    }>RAM AT>FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbData = .;
        *(.bss*)
        . = ALIGN(4);
        _ebData = .;
    }>RAM

    /*
    * unlike in many templates, I do not discard as it's harmless data on
    * the .elf that can be used to debug fuckups in the future
    */
   /* /DISCARD/ : { *(.ARM.attributes) } */
}
/* Placeholder linker script. Replace with your board's actual linker script. */
ENTRY(Reset_Handler)
SECTIONS
{
    . = 0x00000000;
    .text : { *(.text*) }
    .data : { *(.data*) }
    .bss  : { *(.bss*)  }
} 
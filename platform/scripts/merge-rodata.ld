SECTIONS
{
  .text :
  {
    *(.text)
    *(.rodata.* .rodata)
  }
  .rodata : ALIGN(4)
  {
    *(.rodata.* .rodata)
  }
}

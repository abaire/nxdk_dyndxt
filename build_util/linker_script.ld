OUTPUT_FORMAT("pe-i386")
STARTUP(dllmain.o)
SECTIONS
{
    /* Ensure the entry function is at a known offset */
    _entry_point_start  = 0x00000000;
    .entry_point _entry_point_start :
    {
        KEEP(*(.entry_point));
    }
}
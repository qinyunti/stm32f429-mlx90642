ENTRY(reset)
_statck_size = 10240;
MEMORY
{
	FLASH (RX)  : ORIGIN = 0x08000000, LENGTH = 0x00200000
	SRAM1 (RWX) : ORIGIN = 0x20000000, LENGTH = 0x10000
}
SECTIONS
{
	. = ORIGIN(FLASH);

	.text :
	{
		KEEP(*(.vector_table))
		*(.text .text.*)
		*(.rdata .rodata .rodata.*)
		. = ALIGN(4);
		_end_text = .;
	} >FLASH

	.data :
	{
		_start_data = .;
		*(.data .data.*)
		*(.sdata .sdata.*)
		*(.srodata .srodata.*)
		_end_data = .;
	} >SRAM1 AT >FLASH

	PROVIDE(_data_load_addr = LOADADDR(.data));
	PROVIDE(_data_start = ADDR(.data));
	PROVIDE(_data_end = ADDR(.data) + SIZEOF(.data));

	.bss :
	{
		_start_bss = .;
		*(.sbss*)
		*(.bss .bss.*)
		_end_bss = .;
	} >SRAM1

	.heap :
	{
		. = ALIGN(4);
        __heap_start__ = .;
        end = __heap_start__;
        _end = end;
        __end = end;
        KEEP(*(.heap))
        __heap_end__ = .;
        __HeapLimit = __heap_end__;
	} >SRAM1

	.stack :
	{
		. = ORIGIN(SRAM1) + LENGTH(SRAM1) - _statck_size;
		. += _statck_size;
		PROVIDE(_stack_top = .);
	} >SRAM1
}

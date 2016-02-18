SECTIONS
{
	.empty (0x20000000) : {
		__empty_segment = .;
		. = . + 4;
	}
}

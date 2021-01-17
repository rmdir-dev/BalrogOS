#include "stdio.h"
#include "Init.h"
#include "Debug/debug_output.h"
#include "CPU/ports/ports.h"
#include "Memory/memory.h"

/*
Exemple
EXECUTE ASM code file from C
*/
extern int addTwo(int a, int b);

void kernel_main(SMAP_entry* mem_info, uint16_t* mem_entries)
{
	SMAP_entry* info = PHYSICAL_TO_VIRTUAL(mem_info);
	uint16_t* size = PHYSICAL_TO_VIRTUAL(mem_entries);
	initialize_kernel(info, size);
	
	while(1)
	{
	}
}
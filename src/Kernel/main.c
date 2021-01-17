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

void kernel_main(void* mem_info, void* mem_entries)
{
	initialize_kernel();
	uint32_t* info = PHYSICAL_TO_VIRTUAL(mem_info);
	uint16_t* size = PHYSICAL_TO_VIRTUAL(mem_entries);
	printf("%x\n", *(info + 4));
	printf("%x\n", *size);
	
	while(1)
	{
	}
}
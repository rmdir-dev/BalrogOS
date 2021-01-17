#include "stdio.h"
#include "Init.h"
#include "Debug/debug_output.h"
#include "CPU/ports/ports.h"

void kernel_main(void* mem_info, void* mem_entries)
{
	initialize_kernel(mem_info, mem_entries);
	
	while(1)
	{
	}
}
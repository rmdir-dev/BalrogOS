#include "klib/IO/kprint.h"
#include "BalrogOS/Init.h"

void kernel_main(void* mem_info, void* mem_entries)
{
	initialize_kernel(mem_info, mem_entries);
	
	while(1)
	{
	}
}
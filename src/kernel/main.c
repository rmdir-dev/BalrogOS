#include "klib/io/kprint.h"
#include "balrog_os/init.h"

void kernel_main(void* mem_info, void* mem_entries)
{
	initialize_kernel(mem_info, mem_entries);
	
	while(1)
	{
	}
}
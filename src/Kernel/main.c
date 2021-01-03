#include "stdio.h"
#include "Init.h"
#include "Debug/debug_output.h"
#include "CPU/ports/ports.h"

/*
Exemple
EXECUTE ASM code file from C
*/
extern int addTwo(int a, int b);

void kernel_main()
{
	initialize_kernel();

	printf("test test test\n");

	while(1)
	{
	}
}
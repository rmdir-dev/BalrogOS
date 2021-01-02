#include "stdio.h"
#include "Init.h"
#include "Debug/debug_output.h"

/*
Exemple
EXECUTE ASM code file from C
*/
extern int addTwo(int a, int b);

void kernel_main()
{
	initialize_kernel();

	printf("test test test test");

	while(1)
	{
	}
}
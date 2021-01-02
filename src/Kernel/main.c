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

	printf("%x %b %d% %c %s\n", 0x1234, 0b10010101, 25367, 'D', "string");

	while(1)
	{
	}
}
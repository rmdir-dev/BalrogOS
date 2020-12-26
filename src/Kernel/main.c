#include "stdio.h"

void main()
{
	terminal_initialize();

	printf("Kernel loading!\n");
	printf("Test second line %d %s %c\n", 1001, "second string", 'e');
}
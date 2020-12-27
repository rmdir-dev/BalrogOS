#include "stdio.h"
#include "Init.h"

/*
Exemple
EXECUTE ASM code file from C
*/
extern int addTwo(int a, int b);

void main()
{
	initialize_kernel();

	printf("Kernel loading!\n");
	printf("Test second line %d %s %c\n", -1234, "second string", 'e');

	int result = addTwo(5, 7);
	printf("result is: %d", result);

	while(1){}
}
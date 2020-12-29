#include "stdio.h"
#include "Init.h"

/*
Exemple
EXECUTE ASM code file from C
*/
extern int addTwo(int a, int b);
#define KERNEL_LOG(x, ...) printf("\e[0;97m[\e[0;92m  OK  \e[0m] "x"\n", __VA_ARGS__)
#define KERNEL_LOG_MSG(x) printf("\e[0;97m[\e[0;92m  OK  \e[0m] "x"\n")
#define KERNEL_FAILED(x, ...) printf("\e[0;97m[\e[0;94mFAILED\e[0m] "x"\n", __VA_ARGS__)
#define KERNEL_FAILED_MSG(x) printf("\e[0;97m[\e[0;94mFAILED\e[0m] "x"\n")

void kernel_main()
{
	initialize_kernel();

	KERNEL_LOG_MSG("kernel tty intialized.");
	KERNEL_FAILED("test failed %s %d", "string", 5);

	while(1){}
}
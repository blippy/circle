#include "compat.h"

extern CKernel *g_kernel;

#define EOF -1

/*
 * returns a char read as an unsigned char to an int, or EOF on end of file or erro
 */
/*
int getchar()
{

}


char* getsn(char* s, int size)
{
}
*/

void puts(const char* str)
{
	while(*str)
		putchar(*str++);
	//putchar('\r');
	putchar('\n');
}

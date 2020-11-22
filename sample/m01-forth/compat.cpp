#include "kernel.h"
#include "compat.h"

extern CKernel *g_kernel;

#ifndef NULL
#define NULL 0
#endif

#ifndef EOF
#define EOF -1
#endif

extern "C" void _putchar(char character)
{
	g_kernel->m_Screen.Write(&character, 1);
}

extern "C" void putchar(char c)
{
	_putchar(c);
}

int getchar()
{
	static char c;
	int n;
	do {
		n = g_kernel->m_keyb->Read(&c, 1);
	} while (n ==0);
		
	if(c || getchar_echo) _putchar(c);
	return c;
}

void exit(int status)
{
	puts("Exiting. Hanging now");
	while(1);
}

/* reads  the next character from stream and returns it as an 
 * un‐signed char cast to an int, or EOF on end of file or error.
 */
int fgetc(FILE* stream)
{
	return getchar();

}

int feof(FILE* stream) { return 0; }

/*
 * returns a char read as an unsigned char to an int, or EOF on end of file or erro
 */




int getc(FILE* stream) { return fgetc(0); };


/* similar to fgets
 * reads at most onel less that size.
 * Reading stops after an EOF or newline. 
 * If a newline is read, it is stored into the buffer.
 * If a newline is read, it is stored into the buffer.
 * A terminating null byte ('\0') is stored after the last
 * character in the buffer
 */

char* fgets(char* s, int n, FILE *iop)
{
	int c;
	char* cs = s;

	while(--n > 0 && (c = getc(iop)) != EOF)
	{
		// put the input char into the current pointer position, then increment it
		// if a newline entered, break
		if((*cs++ = c) == '\n')
			break;
	}

	*cs = '\0';
	return (c == EOF && cs == s) ? NULL : s;
}

//char* getsn(char* s, int size) { return fgets(s, size, 0); }



void puts(const char* str)
{
	while(*str)
		_putchar(*str++);
	_putchar('\r');
	_putchar('\n');
}

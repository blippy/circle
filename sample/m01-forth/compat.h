#pragma once

inline bool getchar_echo = true;

#ifdef __cplusplus
extern "C" {
#endif



//typedef void* FILE*; // just a dummy at this stage
#define FILE void // just a dummy
	
void puts(const char* str);
char* fgets(char* s, int n, FILE *iop);
int getc(FILE* stream);
int feof(FILE* stream);

void puts(const char* str);
int getchar();

#ifdef __cplusplus
}
#endif


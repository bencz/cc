#ifndef CC_STDLIB_H
#define CC_STDLIB_H

#include <stddef.h>

#define RAND_MAX 0x7FFF
#define RAND32_MAX 0x7FFFFFFF
#define MAX_PATH (260)
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define itoa _itoa
#define ltoa _ltoa

int abs(int n);
double atof(char *str);
int atoi(char *str);
long atol(char *str);
long strtol(const char *nptr, char **endptr, int base);
#ifndef __CC__
long long strtoll(const char *nptr, char **endptr, int base);
#endif
double strtod(const char *nptr, char **endptr);

void qsort(void *base, size_t num, size_t width, int *compare);
void *malloc(size_t size);
void *calloc(size_t num, size_t size);
void *realloc(void *ptr, size_t size);
void exit(int status);
void free(void *ptr);
int system(const char *cmd);

int rand();
void srand(int seed);

char *_itoa(int value, char *str, int radix);
char *_ltoa(long value, char *str, int radix);

#endif

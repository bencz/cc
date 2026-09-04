#ifndef CC_STDIO_H
#define CC_STDIO_H

#include <stddef.h>

#define EOF (-1)

typedef struct _FILE
{
	char *_ptr;
	int _cnt;
	char *_base;
	int _flag;
	int _file;
	int _charbuf;
	int _bufsiz;
	char *_tmpfname;
} FILE;

#if defined(_WIN32)
extern FILE _iob[];
#define stdin (&_iob[0])
#define stdout (&_iob[1])
#define stderr (&_iob[2])
#else
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;
#endif

void perror(const char *s);

int printf(char *format, ...);
int fprintf(FILE *fp, char *format, ...);
int sprintf(char *str, char *format, ...);
#if defined(_WIN32)
#define snprintf _snprintf
int _snprintf(char *str, size_t size, const char *format, ...);
#else
int snprintf(char *str, size_t size, const char *format, ...);
#endif

int scanf(char *format, ...);
int sscanf(const char *str, const char *format, ...);

FILE *fopen(char *filename, char *mode);
int fclose(FILE *stream);
int fflush(FILE *stream);
char *fgets(char *s, int n, FILE *stream);
size_t fread(void *buf, size_t size, size_t n, FILE *fp);
size_t fwrite(const void *buf, size_t size, size_t n, FILE *fp);
int fseek(FILE *stream, long offset, int origin);
long ftell(FILE *stream);
char *gets(char *s);
int getchar();
int putchar(int c);
int puts(const char *s);
int rename(const char *old, const char *new);
int ungetc(int c, FILE *fp);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#endif

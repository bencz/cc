// stdio.h

#define size_t int
#define EOF (-1)
#define NULL ((void*)0)

typedef struct _FILE {
    char* _ptr;
    int _cnt;
    char* _base;
    int _flag;
    int _file;
    int _charbuf;
    int _bufsiz;
    char* _tmpfname;
} FILE;

void perror(const char *s);

int printf(char *format, ...);
int fprintf(FILE *fp, char *format, ...);
int sprintf(char *str, char *format, ...);

int scanf(char *format, ...);
int sscanf(const char *str, const char *format, ...);

FILE *fopen(char *filename, char *mode);
int fclose(FILE *stream);
char *fgets(char *s, int n, FILE *stream);
size_t fread(void *buf, size_t size, size_t n, FILE *fp);
size_t fwrite(const void *buf, size_t size, size_t n, FILE *fp);
char *gets(char *s);
int getchar();
int putchar(int c);
int puts(const char *s);
int rename(const char *old, const char *new);
int ungetc(int c, FILE *fp);

// stdlib.h

#define	RAND_MAX  	0x7FFF
#define	RAND32_MAX  	0x7FFFFFFF
#define	MAX_PATH	(260)

#define long 	int
#define size_t 	int
#define itoa	_itoa
#define ltoa	_ltoa

int	abs(int n);
double	atof(char *str);
int	atoi(char *str);
long	atol(char *str);
double  strtod(const char *nptr, char **endptr); 

void    qsort(void *base, size_t num, size_t width, int *compare);
void    *malloc(size_t size);
void    *calloc(size_t num, size_t size);
void	*realloc(void *ptr, size_t size);
void    exit(int status); 
void    free(void *ptr);
int	system(const char* cmd);

int   rand();
void  srand(int seed);

char*	_itoa (int value, char* str, int radix);
char*	_ltoa (long value, char* str, int radix);


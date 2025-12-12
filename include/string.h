// string.h
#define size_t 		int
#define strdup		_strdup
#define stricmp		_stricmp
#define strnicmp	_strnicmp

int  _stricmp(const char* str1, const char* str2);
int  _strnicmp(const char *s1, const char *s2, size_t n);
int  strlen(const char* s);
char *strcpy(char* dst, const char* src);
char *strncpy(char* dst, const char* src, size_t n);
char *strchr(const char* str, int chr);
char *strrchr(const char* str, int chr);
int  strcmp(const char* str1, const char* str2);
int  strncmp(const char *s1, const char *s2, size_t n);
char *strstr(const char *s1, const char *s2);
char *strtok(char *s1, const char *s2);
void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *buf, int ch, size_t n);

char *_strdup(const char* str);

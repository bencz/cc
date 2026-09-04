#ifndef CC_STDDEF_H
#define CC_STDDEF_H

typedef unsigned int size_t;
typedef int ptrdiff_t;
typedef unsigned short wchar_t;

#define NULL ((void *)0)
#define offsetof(type, member) ((size_t)&(((type *)0)->member))

#endif

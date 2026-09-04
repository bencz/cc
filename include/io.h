#ifndef CC_IO_H
#define CC_IO_H

typedef struct __finddata_t
{
	int attrib;
	int time_create;
	int time_access;
	int time_write;
	int size;
	char name[260];
} _finddata_t;

#define _O_RDONLY 0
#define _O_WRONLY 1
#define _O_RDWR 2
#define _O_TEXT 0x4000
#define _O_BINARY 0x8000

int open(char *filename, int amode, ...);
long _findfirst(char *pattern, _finddata_t *data);
long _findnext(long handle, _finddata_t *data);
int _findclose(long handle);

#define _A_NORMAL 0x00
#define _A_RDONLY 0x01
#define _A_HIDDEN 0x02
#define _A_SYSTEM 0x04
#define _A_VOLID 0x08
#define _A_SUBDIR 0x10
#define _A_ARCH 0x20

#endif

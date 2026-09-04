/* Host operating-system services used by the compiler driver. */

#ifndef CC_PLATFORM_H
#define CC_PLATFORM_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX_PATH
#define MAX_PATH 4096
#endif

typedef struct _PLATFORM_FINDDATA
{
	char name[MAX_PATH];
} PLATFORM_FINDDATA;

typedef struct _PLATFORM_FINDHANDLE
{
	void *state;
} PLATFORM_FINDHANDLE;

int platform_findfirst(const char *pattern, PLATFORM_FINDDATA *data, PLATFORM_FINDHANDLE *handle);
int platform_findnext(PLATFORM_FINDDATA *data, PLATFORM_FINDHANDLE *handle);
void platform_findclose(PLATFORM_FINDHANDLE *handle);
int platform_get_module_path(char *buffer, int size);
int platform_resolve_system_library(const char *name, char *buffer, int size);

#endif /* CC_PLATFORM_H */

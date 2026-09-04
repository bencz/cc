/* Windows host services. */

#include <io.h>
#include <windows.h>

#include "cc_platform.h"

#if !defined(_MSC_VER)
DWORD WINAPI SearchPathA(const char *path,
                         const char *fileName,
                         const char *extension,
                         DWORD bufferLength,
                         char *buffer,
                         char **filePart);
#endif

typedef struct _WindowsFindState
{
	intptr_t handle;
	struct _finddata_t data;
} WindowsFindState;

static void copyFindName(char *destination, const char *source)
{
	size_t length = strlen(source);
	if (length >= MAX_PATH)
	{
		fprintf(stderr, "platform: matched path component is too long\n");
		exit(EXIT_FAILURE);
	}
	memcpy(destination, source, length + 1U);
}

int platform_findfirst(const char *pattern, PLATFORM_FINDDATA *data, PLATFORM_FINDHANDLE *handle)
{
	WindowsFindState *state = calloc(1U, sizeof(*state));
	if (state == NULL)
	{
		fprintf(stderr, "platform: out of memory\n");
		exit(EXIT_FAILURE);
	}
	state->handle = _findfirst(pattern, &state->data);
	if (state->handle == -1)
	{
		free(state);
		handle->state = NULL;
		return -1;
	}
	handle->state = state;
	copyFindName(data->name, state->data.name);
	return 0;
}

int platform_findnext(PLATFORM_FINDDATA *data, PLATFORM_FINDHANDLE *handle)
{
	WindowsFindState *state = handle->state;
	if (state == NULL || _findnext(state->handle, &state->data) != 0)
	{
		return -1;
	}
	copyFindName(data->name, state->data.name);
	return 0;
}

void platform_findclose(PLATFORM_FINDHANDLE *handle)
{
	WindowsFindState *state = handle->state;
	if (state != NULL)
	{
		_findclose(state->handle);
		free(state);
		handle->state = NULL;
	}
}

int platform_get_module_path(char *buffer, int size)
{
	DWORD length = GetModuleFileNameA(NULL, buffer, (DWORD)size);
	return length > 0 && length < (DWORD)size ? (int)length : 0;
}

int platform_resolve_system_library(const char *name, char *buffer, int size)
{
	DWORD length = SearchPathA(NULL, name, NULL, (DWORD)size, buffer, NULL);
	return length > 0 && length < (DWORD)size;
}

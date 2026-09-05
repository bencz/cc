/* Linux host services. */

#define _POSIX_C_SOURCE 200809L
#include "cc_platform.h"

#include <glob.h>
#include <unistd.h>

typedef struct _PosixFindState
{
	glob_t matches;
	size_t index;
} PosixFindState;

static void copyMatchName(char *destination, const char *path)
{
	const char *name = strrchr(path, '/');
	size_t length;
	name = name == NULL ? path : name + 1;
	length = strlen(name);
	if (length >= MAX_PATH)
	{
		fprintf(stderr, "platform: matched path component is too long\n");
		exit(EXIT_FAILURE);
	}
	memcpy(destination, name, length + 1U);
}

int platform_findfirst(const char *pattern, PLATFORM_FINDDATA *data, PLATFORM_FINDHANDLE *handle)
{
	PosixFindState *state = calloc(1U, sizeof(*state));
	if (state == NULL)
	{
		fprintf(stderr, "platform: out of memory\n");
		exit(EXIT_FAILURE);
	}
	if (glob(pattern, GLOB_NOSORT, NULL, &state->matches) != 0 || state->matches.gl_pathc == 0)
	{
		globfree(&state->matches);
		free(state);
		handle->state = NULL;
		return -1;
	}
	handle->state = state;
	copyMatchName(data->name, state->matches.gl_pathv[0]);
	return 0;
}

int platform_findnext(PLATFORM_FINDDATA *data, PLATFORM_FINDHANDLE *handle)
{
	PosixFindState *state = handle->state;
	if (state == NULL || ++state->index >= state->matches.gl_pathc)
	{
		return -1;
	}
	copyMatchName(data->name, state->matches.gl_pathv[state->index]);
	return 0;
}

void platform_findclose(PLATFORM_FINDHANDLE *handle)
{
	PosixFindState *state = handle->state;
	if (state != NULL)
	{
		globfree(&state->matches);
		free(state);
		handle->state = NULL;
	}
}

int platform_get_module_path(char *buffer, int size)
{
	ssize_t length = readlink("/proc/self/exe", buffer, (size_t)size - 1U);
	if (length < 0 || length >= size)
	{
		return 0;
	}
	buffer[length] = '\0';
	return (int)length;
}

int platform_resolve_system_library(const char *name, char *buffer, int size)
{
	(void)name;
	(void)buffer;
	(void)size;
	return 0;
}

/*
 * cc_platform.c - Implementacao das funcoes Cross-Platform
 */

#include "cc_platform.h"

BackendType g_backend = BACKEND_X86_PE;

/*============================================================================
 * Funcoes de busca de arquivos
 *============================================================================*/

int platform_findfirst(const char *pattern, PLATFORM_FINDDATA *fdata, PLATFORM_FINDHANDLE *fhandle)
{
#ifdef PLATFORM_WINDOWS
    fhandle->handle = _findfirst(pattern, &fhandle->wdata);
    if (fhandle->handle == -1) return -1;
    strcpy(fdata->name, fhandle->wdata.name);
    return 0;
#else
    fhandle->current_index = 0;
    int result = glob(pattern, GLOB_NOSORT, NULL, &fhandle->glob_result);
    if (result != 0 || fhandle->glob_result.gl_pathc == 0) {
        fhandle->valid = 0;
        return -1;
    }
    fhandle->valid = 1;
    const char *fullpath = fhandle->glob_result.gl_pathv[0];
    const char *basename_ptr = strrchr(fullpath, '/');
    if (basename_ptr) {
        strcpy(fdata->name, basename_ptr + 1);
    } else {
        strcpy(fdata->name, fullpath);
    }
    return 0;
#endif
}

int platform_findnext(PLATFORM_FINDDATA *fdata, PLATFORM_FINDHANDLE *fhandle)
{
#ifdef PLATFORM_WINDOWS
    if (_findnext(fhandle->handle, &fhandle->wdata) != 0) return -1;
    strcpy(fdata->name, fhandle->wdata.name);
    return 0;
#else
    fhandle->current_index++;
    if ((size_t)fhandle->current_index >= fhandle->glob_result.gl_pathc) return -1;
    const char *fullpath = fhandle->glob_result.gl_pathv[fhandle->current_index];
    const char *basename_ptr = strrchr(fullpath, '/');
    if (basename_ptr) {
        strcpy(fdata->name, basename_ptr + 1);
    } else {
        strcpy(fdata->name, fullpath);
    }
    return 0;
#endif
}

void platform_findclose(PLATFORM_FINDHANDLE *fhandle)
{
#ifdef PLATFORM_WINDOWS
    if (fhandle->handle != -1) _findclose(fhandle->handle);
#else
    if (fhandle->valid) globfree(&fhandle->glob_result);
#endif
}

int platform_get_module_path(char *buffer, int size)
{
#ifdef PLATFORM_WINDOWS
    return GetModuleFileName(NULL, buffer, size);
#elif defined(PLATFORM_MACOS)
    char temp[MAX_PATH];
    uint32_t bufsize = sizeof(temp);
    if (_NSGetExecutablePath(temp, &bufsize) == 0) {
        char *resolved = realpath(temp, buffer);
        return resolved ? (int)strlen(buffer) : 0;
    }
    return 0;
#else
    ssize_t len = readlink("/proc/self/exe", buffer, size - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return (int)len;
    }
    return 0;
#endif
}

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <stdio.h>

typedef int(WINAPI *ValueFunction)(int);

int main(int argc, char **argv)
{
	void *reservation;
	HMODULE module;
	FARPROC address;
	ValueFunction value;
	int result;
	if (argc != 2)
	{
		return 1;
	}
	reservation = VirtualAlloc((void *)0x10000000, 0x01000000, MEM_RESERVE, PAGE_NOACCESS);
	if (reservation == NULL)
	{
		return 2;
	}
	module = LoadLibraryA(argv[1]);
	if (module == NULL)
	{
		fprintf(stderr, "LoadLibrary failed: %lu\n", GetLastError());
		VirtualFree(reservation, 0, MEM_RELEASE);
		return 3;
	}
	address = GetProcAddress(module, "dll_value");
	memcpy(&value, &address, sizeof(value));
	result = value != NULL && value(7) == 49 ? 0 : 4;
	FreeLibrary(module);
	VirtualFree(reservation, 0, MEM_RELEASE);
	return result;
}

/*
 * cc_platform.h - Abstracoes Cross-Platform
 * 
 * Este arquivo fornece compatibilidade entre Windows, Linux e macOS,
 * abstraindo funcoes e tipos especificos de cada sistema operacional.
 */

#ifndef CC_PLATFORM_H
#define CC_PLATFORM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/*============================================================================
 * Deteccao de Plataforma
 *============================================================================*/

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
#elif defined(__APPLE__) && defined(__MACH__)
    #define PLATFORM_MACOS 1
    #define PLATFORM_UNIX 1
#elif defined(__linux__)
    #define PLATFORM_LINUX 1
    #define PLATFORM_UNIX 1
#else
    #define PLATFORM_UNIX 1
#endif

/*============================================================================
 * Includes especificos de plataforma
 *============================================================================*/

#ifdef PLATFORM_WINDOWS
    #include <windows.h>
    #include <io.h>
#else
    #include <unistd.h>
    #include <dirent.h>
    #include <sys/stat.h>
    #include <limits.h>
    #include <libgen.h>
    #include <glob.h>
#endif

#ifdef PLATFORM_MACOS
    #include <mach-o/dyld.h>
#endif

/*============================================================================
 * Tipos Cross-Platform
 *============================================================================*/

#ifndef PLATFORM_WINDOWS
    typedef unsigned char  BYTE;
    typedef unsigned short WORD;
    typedef unsigned int   DWORD;
    typedef unsigned long  ULONG;
    
    #ifndef MAX_PATH
        #ifdef PATH_MAX
            #define MAX_PATH PATH_MAX
        #else
            #define MAX_PATH 4096
        #endif
    #endif
#endif

/*============================================================================
 * Estruturas para PE (Portable Executable) - necessarias para backend x86
 * Definidas apenas quando nao estamos no Windows
 *============================================================================*/

#ifndef PLATFORM_WINDOWS

#pragma pack(push, 1)

typedef struct _IMAGE_DOS_HEADER {
    WORD   e_magic;
    WORD   e_cblp;
    WORD   e_cp;
    WORD   e_crlc;
    WORD   e_cparhdr;
    WORD   e_minalloc;
    WORD   e_maxalloc;
    WORD   e_ss;
    WORD   e_sp;
    WORD   e_csum;
    WORD   e_ip;
    WORD   e_cs;
    WORD   e_lfarlc;
    WORD   e_ovno;
    WORD   e_res[4];
    WORD   e_oemid;
    WORD   e_oeminfo;
    WORD   e_res2[10];
    DWORD  e_lfanew;
} IMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER {
    WORD    Machine;
    WORD    NumberOfSections;
    DWORD   TimeDateStamp;
    DWORD   PointerToSymbolTable;
    DWORD   NumberOfSymbols;
    WORD    SizeOfOptionalHeader;
    WORD    Characteristics;
} IMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
    DWORD   VirtualAddress;
    DWORD   Size;
} IMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16

typedef struct _IMAGE_OPTIONAL_HEADER {
    WORD    Magic;
    BYTE    MajorLinkerVersion;
    BYTE    MinorLinkerVersion;
    DWORD   SizeOfCode;
    DWORD   SizeOfInitializedData;
    DWORD   SizeOfUninitializedData;
    DWORD   AddressOfEntryPoint;
    DWORD   BaseOfCode;
    DWORD   BaseOfData;
    DWORD   ImageBase;
    DWORD   SectionAlignment;
    DWORD   FileAlignment;
    WORD    MajorOperatingSystemVersion;
    WORD    MinorOperatingSystemVersion;
    WORD    MajorImageVersion;
    WORD    MinorImageVersion;
    WORD    MajorSubsystemVersion;
    WORD    MinorSubsystemVersion;
    DWORD   Win32VersionValue;
    DWORD   SizeOfImage;
    DWORD   SizeOfHeaders;
    DWORD   CheckSum;
    WORD    Subsystem;
    WORD    DllCharacteristics;
    DWORD   SizeOfStackReserve;
    DWORD   SizeOfStackCommit;
    DWORD   SizeOfHeapReserve;
    DWORD   SizeOfHeapCommit;
    DWORD   LoaderFlags;
    DWORD   NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER;

typedef struct _IMAGE_NT_HEADERS {
    DWORD Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER OptionalHeader;
} IMAGE_NT_HEADERS;

#define IMAGE_SIZEOF_SHORT_NAME 8

typedef struct _IMAGE_SECTION_HEADER {
    BYTE    Name[IMAGE_SIZEOF_SHORT_NAME];
    union {
        DWORD   PhysicalAddress;
        DWORD   VirtualSize;
    } Misc;
    DWORD   VirtualAddress;
    DWORD   SizeOfRawData;
    DWORD   PointerToRawData;
    DWORD   PointerToRelocations;
    DWORD   PointerToLinenumbers;
    WORD    NumberOfRelocations;
    WORD    NumberOfLinenumbers;
    DWORD   Characteristics;
} IMAGE_SECTION_HEADER;

typedef struct _IMAGE_EXPORT_DIRECTORY {
    DWORD   Characteristics;
    DWORD   TimeDateStamp;
    WORD    MajorVersion;
    WORD    MinorVersion;
    DWORD   Name;
    DWORD   Base;
    DWORD   NumberOfFunctions;
    DWORD   NumberOfNames;
    DWORD   AddressOfFunctions;
    DWORD   AddressOfNames;
    DWORD   AddressOfNameOrdinals;
} IMAGE_EXPORT_DIRECTORY;

#pragma pack(pop)

typedef void* HMODULE;

#endif /* !PLATFORM_WINDOWS */

/*============================================================================
 * Estrutura para iteracao de diretorio
 *============================================================================*/

typedef struct _PLATFORM_FINDDATA {
    char name[MAX_PATH];
} PLATFORM_FINDDATA;

typedef struct _PLATFORM_FINDHANDLE {
#ifdef PLATFORM_WINDOWS
    intptr_t handle;
    struct _finddata_t wdata;
#else
    glob_t glob_result;
    int current_index;
    int valid;
#endif
} PLATFORM_FINDHANDLE;

/*============================================================================
 * Funcoes Cross-Platform - Declaracoes
 *============================================================================*/

int platform_findfirst(const char *pattern, PLATFORM_FINDDATA *fdata, PLATFORM_FINDHANDLE *fhandle);
int platform_findnext(PLATFORM_FINDDATA *fdata, PLATFORM_FINDHANDLE *fhandle);
void platform_findclose(PLATFORM_FINDHANDLE *fhandle);
int platform_get_module_path(char *buffer, int size);

/*============================================================================
 * Backend Selection
 *============================================================================*/

typedef enum {
    BACKEND_X86_PE = 0,
    BACKEND_HLASM
} BackendType;

extern BackendType g_backend;

#endif /* CC_PLATFORM_H */

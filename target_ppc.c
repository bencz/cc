/* Big-endian ILP32 PowerPC assembly targets. The system toolchain links libc. */
#include "cc.h"
#include "ppc.h"

static void definePowerMacros(CompilerContext *context)
{
	hashPut("__powerpc__", "1", &context->sources.hash);
	hashPut("__PPC__", "1", &context->sources.hash);
	hashPut("__ILP32__", "1", &context->sources.hash);
	hashPut("__CHAR_UNSIGNED__", "1", &context->sources.hash);
	hashPut("__BIG_ENDIAN__", "1", &context->sources.hash);
	hashPut("__BYTE_ORDER__", "4321", &context->sources.hash);
	hashPut("__ORDER_BIG_ENDIAN__", "4321", &context->sources.hash);
	hashPut("__ORDER_LITTLE_ENDIAN__", "1234", &context->sources.hash);
}

static void defineLinuxMacros(CompilerContext *context)
{
	definePowerMacros(context);
	hashPut("__linux__", "1", &context->sources.hash);
	hashPut("__unix__", "1", &context->sources.hash);
	hashPut("__ELF__", "1", &context->sources.hash);
}

static void defineAixMacros(CompilerContext *context)
{
	definePowerMacros(context);
	hashPut("_AIX", "1", &context->sources.hash);
	hashPut("_ARCH_PPC", "1", &context->sources.hash);
	hashPut("__unix__", "1", &context->sources.hash);
}

static void useSystemRuntime(CompilerContext *context)
{
	/* crt0 and libc belong to the target linker, not to a translation unit. */
	(void)context;
}

static unsigned char encodeByte(CompilerContext *context, unsigned char value)
{
	(void)context;
	return value;
}

static void emitLinux(CompilerContext *context, const char *path)
{
	(void)context;
	ppcEmitModule(path, &ppcSysvAbi);
}

static void emitAix(CompilerContext *context, const char *path)
{
	(void)context;
	ppcEmitModule(path, &ppcAixAbi);
}

const TargetDescriptor targetPpcLinux = {"ppc32-linux",
                                         ".s",
                                         NULL,
                                         {4, 4, 2, 4, 4, 4, 8, 0, 0, 1},
                                         0,
                                         1,
                                         0,
                                         1,
                                         defineLinuxMacros,
                                         useSystemRuntime,
                                         useSystemRuntime,
                                         encodeByte,
                                         emitLinux};

const TargetDescriptor targetPpcAix = {"ppc32-aix",
                                       ".s",
                                       NULL,
                                       {4, 4, 2, 4, 4, 4, 8, 0, 4, 1},
                                       0,
                                       1,
                                       0,
                                       1,
                                       defineAixMacros,
                                       useSystemRuntime,
                                       useSystemRuntime,
                                       encodeByte,
                                       emitAix};

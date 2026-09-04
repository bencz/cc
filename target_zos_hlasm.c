/* z/OS AMODE 31 Language Environment target definition. */

#include "cc.h"

static void definePredefinedMacros(CompilerContext *context)
{
	hashPut("__MVS__", "1", &context->sources.hash);
	hashPut("__370__", "1", &context->sources.hash);
	hashPut("__s390__", "1", &context->sources.hash);
	hashPut("__ILP32__", "1", &context->sources.hash);
}

static void addRuntimePrelude(CompilerContext *context)
{
	(void)context;
}

static void addTargetStartup(CompilerContext *context)
{
	(void)context;
	if (mcc.typeApp != 3)
	{
		return;
	}
	preproAddSyntheticLine("int _main(void) {", 3);
	preproAddSyntheticLine("  return main();", 4);
	preproAddSyntheticLine("}", 5);
}

static unsigned char encodeExecutionByte(CompilerContext *context, unsigned char value)
{
	(void)context;
	return hlasmExecutionByte(value);
}

static void emitOutput(CompilerContext *context, const char *outputFile)
{
	(void)context;
	hlasm_link(outputFile);
	if (cmd.jclfile[0] != '\0')
	{
		zos_write_jcl(outputFile, cmd.jclfile);
	}
}

const TargetDescriptor targetZosHlasm = {
    "zos-hlasm",
    ".asm",
    NULL,
    {4, 4, 2, 4, 4, 4, 8, 0},
    0,
    1,
    1,
    1,
    definePredefinedMacros,
    addRuntimePrelude,
    addTargetStartup,
    encodeExecutionByte,
    emitOutput,
};

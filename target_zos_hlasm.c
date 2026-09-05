/* z/OS AMODE 31 Language Environment target definition. */

#include "cc.h"

static void definePredefinedMacros(CompilerContext *context)
{
	hashPut("__MVS__", "1", &context->sources.hash);
	hashPut("__370__", "1", &context->sources.hash);
	hashPut("__s390__", "1", &context->sources.hash);
	hashPut("__ILP32__", "1", &context->sources.hash);
	hashPut("__BFP__", "1", &context->sources.hash);
	hashPut("__CHAR_UNSIGNED__", "1", &context->sources.hash);
	hashPut("_CHAR_UNSIGNED", "1", &context->sources.hash);
}

static void addRuntimePrelude(CompilerContext *context)
{
	(void)context;
	preproAddSyntheticLine("void __cc_zos_ieee_init(void);", 1);
}

const char *zosRuntimeSource(void)
{
	return "/* Compile with the IBM headers and FLOAT(IEEE), NOXPLINK, NORENT. */\n"
	       "#include <_Ieee754.h>\n"
	       "void __cc_zos_ieee_init(void)\n"
	       "{\n"
	       "    __fp_setmode(_FP_BFP_MODE);\n"
	       "}\n";
}

static void addTargetStartup(CompilerContext *context)
{
	(void)context;
	if (mcc.typeApp != 3)
	{
		return;
	}
	preproAddSyntheticLine("int _main(void) {", 3);
	preproAddSyntheticLine("  __cc_zos_ieee_init();", 4);
	preproAddSyntheticLine("  return main();", 5);
	preproAddSyntheticLine("}", 6);
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
	if (mcc.typeApp == 3)
	{
		char runtimePath[MAX_PATH];
		FILE *runtime;
		int length = snprintf(runtimePath, sizeof(runtimePath), "%s.runtime.c", outputFile);
		if (length < 0 || (size_t)length >= sizeof(runtimePath))
		{
			error("z/OS runtime", "runtime companion path is too long");
		}
		runtime = fopen(runtimePath, "w");
		if (runtime == NULL)
		{
			error("z/OS runtime", "cannot create '%s'", runtimePath);
		}
		if (fputs(zosRuntimeSource(), runtime) < 0 || fclose(runtime) != 0)
		{
			error("z/OS runtime", "cannot write '%s'", runtimePath);
		}
	}
	if (cmd.jclfile[0] != '\0')
	{
		zos_write_jcl(outputFile, cmd.jclfile);
	}
}

const TargetDescriptor targetZosHlasm = {
    "zos-hlasm",
    ".asm",
    NULL,
    {4, 4, 2, 4, 4, 4, 8, 0, 0, 1},
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

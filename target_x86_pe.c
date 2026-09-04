/* IA-32 PE target definition. */

#include "cc.h"

static void definePredefinedMacros(CompilerContext *context)
{
	hashPut("_WIN32", "1", &context->sources.hash);
	hashPut("__i386__", "1", &context->sources.hash);
	hashPut("__ILP32__", "1", &context->sources.hash);
}

static void addRuntimePrelude(CompilerContext *context)
{
	(void)context;
	preproAddSyntheticLine("short _RoundNear = 0x137F;", 1);
	preproAddSyntheticLine("short _RoundChop = 0x1F7F;", 2);
}

static void addTargetStartup(CompilerContext *context)
{
	int line = 3;
	(void)context;
	preproAddSyntheticLine("void _main() {", line++);
	if (mcc.typeApp == 3)
	{
		preproAddSyntheticLine("  char **argv, **env;", line++);
		preproAddSyntheticLine("  int argc, new_mode = 0;", line++);
		preproAddSyntheticLine("  __set_app_type(1);", line++);
		preproAddSyntheticLine("  _controlfp(0x10000, 0x30000);", line++);
		preproAddSyntheticLine("  __getmainargs(&argc, &argv, &env, 0, &new_mode);", line++);
		preproAddSyntheticLine("  exit(main(argc, argv, env));", line++);
	}
	else if (mcc.typeApp == 2)
	{
		preproAddSyntheticLine("  STARTUPINFOA si;", line++);
		preproAddSyntheticLine("  __set_app_type(2);", line++);
		preproAddSyntheticLine("  _controlfp(0x10000, 0x30000);", line++);
		preproAddSyntheticLine("  char *p = GetCommandLineA();", line++);
		preproAddSyntheticLine("  int c = (*p++ == '\"') ? '\"' : ' ';", line++);
		preproAddSyntheticLine("  while (*p != c && *p != 0) p++;", line++);
		preproAddSyntheticLine("  if (*p != 0) p++;", line++);
		preproAddSyntheticLine("  while (*p <= ' ' && *p != 0) p++;", line++);
		preproAddSyntheticLine("  GetStartupInfoA(&si);", line++);
		preproAddSyntheticLine("  exit(WinMain(GetModuleHandleA((void*)0), (void*)0,", line++);
		preproAddSyntheticLine("       p, (si.dwFlags&1) ? si.wShowWindow : 10));", line++);
	}
	else
	{
		preproAddSyntheticLine("  return 1;", line++);
	}
	preproAddSyntheticLine("}", line);
}

static unsigned char encodeExecutionByte(CompilerContext *context, unsigned char value)
{
	(void)context;
	return value;
}

static void emitOutput(CompilerContext *context, const char *outputFile)
{
	(void)context;
	(void)outputFile;
	x86LowerIr();
	link();
}

const TargetDescriptor targetX86Pe = {
    "x86-pe",
    ".exe",
    ".dll",
    {4, 4, 2, 4, 4, 4, 8, 1},
    1,
    0,
    0,
    0,
    definePredefinedMacros,
    addRuntimePrelude,
    addTargetStartup,
    encodeExecutionByte,
    emitOutput,
};

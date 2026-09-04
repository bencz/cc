#include "cc.h"

CompilerContext compiler;

typedef struct _SimpleOption
{
	const char *name;
	int flag;
} SimpleOption;

static const SimpleOption simpleOptions[] = {
    {"-E", oSRC},
    {"-shared", oDLL},
    {"-token", oTOKEN},
    {"-asm", oASM},
    {"--lines", oLINES},
    {"-trace", oTRACE},
    {"-debug", oDEBUG},
    {"-name", oNAME},
    {"--opt", oUOPT},
};

static void printUsage(void)
{
	printf("%s",
	       "Usage: cc [options] file...\n"
	       "Options:\n"
	       "  -D<macro>[=value]       Define a preprocessor macro\n"
	       "  --target=<target>       x86-pe or zos-hlasm\n"
	       "  -shared                 Generate a DLL file (Windows only)\n"
	       "  -o <file>               Set the output file name\n"
	       "  --emit-ir=<file>        Write the verified target-neutral IR\n"
	       "  -l<name>                Link with <name>.dll (Windows only)\n"
	       "  -L<directory>           Add a PE import-library search directory\n"
	       "  --pe-sysroot=<dir>      Set the root used to find PE DLLs or DEF files\n"
	       "  -hlasm                  Alias for --target=zos-hlasm\n"
	       "  -S                      Emit target assembly (z/OS HLASM target)\n"
	       "  --exec-charset=<name>   ibm-1047 or ibm-037\n"
	       "  --zos-jcl=<file>        Write a self-contained assemble/bind/run JCL job\n"
	       "  -Werror                 Treat source warnings as errors\n"
	       "  -Wno-error              Allow source warnings\n"
	       "  -E                      Print preprocessed source\n"
	       "  -token                  Print the token list\n"
	       "  -asm                    Generate an x86 assembly listing\n"
	       "  --lines                 Print the source line count\n");
	exit(0);
}

static void copyOption(char *destination, size_t capacity, const char *source, const char *option)
{
	size_t length = strlen(source);
	if (length >= capacity)
	{
		error("main", "%s value is too long", option);
	}
	memcpy(destination, source, length + 1U);
}

static void tracePhase(const char *message)
{
	if (!(opt & oDEBUG))
	{
		return;
	}
	fprintf(stderr, "cc: %s\n", message);
	fflush(stderr);
}

static void initializeCommand(void)
{
	const char *defaultImports = "msvcrt.dll;kernel32.dll;user32.dll;gdi32.dll;";
	int pathLength;
	char *lastSeparator;

	cmd.target = targetDefault();
	cmd.executionCharset = EXEC_CHARSET_IBM1047;
	cmd.warningsAsErrors = 1;
	irModuleInit(&compiler.ir);
	irBuilderInit(&compiler.irBuilder, &compiler.ir);
	mcc.nPreFile = -1;
	copyOption(cmd.impfiles, sizeof(cmd.impfiles), defaultImports, "default import list");
	pathLength = platform_get_module_path(cmd.MCCDIR, MAX_PATH);
	if (pathLength <= 0)
	{
		copyOption(cmd.MCCDIR, sizeof(cmd.MCCDIR), ".", "compiler directory");
		return;
	}

	lastSeparator = strrchr(cmd.MCCDIR, '/');
	if (lastSeparator == NULL)
	{
		lastSeparator = strrchr(cmd.MCCDIR, '\\');
	}
	if (lastSeparator != NULL)
	{
		*lastSeparator = '\0';
	}
}

static void appendImportLibrary(const char *name)
{
	size_t used = strlen(cmd.impfiles);
	size_t nameLength = strlen(name);
	if (nameLength == 0U)
	{
		error("main", "-l requires a library name");
	}
	if (used + nameLength + 6U > sizeof(cmd.impfiles))
	{
		error("main", "import library list is too large");
	}
	memcpy(cmd.impfiles + used, name, nameLength);
	memcpy(cmd.impfiles + used + nameLength, ".dll;", 6U);
}

static void recordDefine(char *definition)
{
	if (cmd.nDefines >= (int)(sizeof(cmd.defines) / sizeof(cmd.defines[0])))
	{
		error("main", "too many -D options");
	}
	cmd.defines[cmd.nDefines++] = definition;
}

static void recordLibraryPath(char *path)
{
	if (*path == '\0')
	{
		error("main", "-L requires a directory");
	}
	if (cmd.libraryPathCount == cmd.libraryPathCapacity)
	{
		int capacity = cmd.libraryPathCapacity == 0 ? 8 : cmd.libraryPathCapacity * 2;
		cmd.libraryPaths = xrealloc(cmd.libraryPaths, (size_t)capacity * sizeof(*cmd.libraryPaths));
		cmd.libraryPathCapacity = capacity;
	}
	cmd.libraryPaths[cmd.libraryPathCount++] = path;
}

static int parseCommandLine(int argc, char *argv[])
{
	const int optionCount = (int)(sizeof(simpleOptions) / sizeof(simpleOptions[0]));
	int emitAssembly = 0;
	int argumentIndex;

	if (argc == 1)
	{
		printUsage();
	}

	for (argumentIndex = 1; argumentIndex < argc; ++argumentIndex)
	{
		char *argument = argv[argumentIndex];
		int optionIndex;
		for (optionIndex = 0; optionIndex < optionCount; ++optionIndex)
		{
			if (strcmp(argument, simpleOptions[optionIndex].name) == 0)
			{
				break;
			}
		}

		if (optionIndex < optionCount)
		{
			opt |= simpleOptions[optionIndex].flag;
		}
		else if (strcmp(argument, "-hlasm") == 0 || strcmp(argument, "--target=zos-hlasm") == 0)
		{
			cmd.target = &targetZosHlasm;
		}
		else if (strcmp(argument, "--target=x86-pe") == 0)
		{
			cmd.target = &targetX86Pe;
		}
		else if (strncmp(argument, "--target=", 9) == 0)
		{
			cmd.target = targetFind(argument + 9);
			if (cmd.target == NULL)
			{
				error("main", "unknown target '%s'", argument + 9);
			}
		}
		else if (strcmp(argument, "-S") == 0)
		{
			emitAssembly = 1;
		}
		else if (strcmp(argument, "-Werror") == 0)
		{
			cmd.warningsAsErrors = 1;
		}
		else if (strcmp(argument, "-Wno-error") == 0)
		{
			cmd.warningsAsErrors = 0;
		}
		else if (strcmp(argument, "--exec-charset=ibm-1047") == 0)
		{
			cmd.executionCharset = EXEC_CHARSET_IBM1047;
		}
		else if (strcmp(argument, "--exec-charset=ibm-037") == 0)
		{
			cmd.executionCharset = EXEC_CHARSET_IBM037;
		}
		else if (strncmp(argument, "--exec-charset=", 15) == 0)
		{
			error("main", "unsupported execution charset '%s'", argument + 15);
		}
		else if (strncmp(argument, "--zos-jcl=", 10) == 0)
		{
			copyOption(cmd.jclfile, sizeof(cmd.jclfile), argument + 10, "--zos-jcl");
		}
		else if (strncmp(argument, "--emit-ir=", 10) == 0)
		{
			copyOption(cmd.irfile, sizeof(cmd.irfile), argument + 10, "--emit-ir");
		}
		else if (strncmp(argument, "--pe-sysroot=", 13) == 0)
		{
			copyOption(cmd.peSysroot,
			           sizeof(cmd.peSysroot),
			           argument + 13,
			           "--pe-sysroot");
		}
		else if (strcmp(argument, "-o") == 0)
		{
			if (++argumentIndex >= argc)
			{
				error("main", "-o requires a file name");
			}
			copyOption(cmd.outfile, sizeof(cmd.outfile), argv[argumentIndex], "-o");
		}
		else if (strncmp(argument, "-l", 2) == 0)
		{
			appendImportLibrary(argument + 2);
		}
		else if (strncmp(argument, "-L", 2) == 0)
		{
			char *path = argument + 2;
			if (*path == '\0')
			{
				if (++argumentIndex >= argc)
				{
					error("main", "-L requires a directory");
				}
				path = argv[argumentIndex];
			}
			recordLibraryPath(path);
		}
		else if (strncmp(argument, "-D", 2) == 0)
		{
			char *definition = argument + 2;
			if (*definition == '\0')
			{
				if (++argumentIndex >= argc)
				{
					error("main", "-D requires an argument");
				}
				definition = argv[argumentIndex];
			}
			recordDefine(definition);
		}
		else if (argument[0] != '-')
		{
			if (cmd.nSrc >= (int)(sizeof(cmd.srcfile) / sizeof(cmd.srcfile[0])))
			{
				error("main", "too many input files");
			}
			cmd.srcfile[cmd.nSrc++] = argument;
		}
		else
		{
			error("main", "invalid option -- '%s'", argument);
		}
	}
	return emitAssembly;
}

static void validateCommandLine(int emitAssembly)
{
	targetValidate(cmd.target);
	if (cmd.nSrc == 0)
	{
		error("main", "no source files to build");
	}
	if (emitAssembly && !cmd.target->supportsAssemblyOutput)
	{
		error("main", "-S currently requires --target=zos-hlasm");
	}
	if (cmd.jclfile[0] != '\0' && !cmd.target->supportsJclOutput)
	{
		error("main", "--zos-jcl requires --target=zos-hlasm");
	}
	if (cmd.target->requiresSingleTranslationUnit && cmd.nSrc != 1)
	{
		error("main",
		      "z/OS emits one translation unit per invocation; provide exactly one source file");
	}
	if ((opt & oDLL) && !cmd.target->supportsSharedOutput)
	{
		error("main", "target '%s' does not support shared output", cmd.target->name);
	}
}

static void applyCommandLineDefines(void)
{
	int defineIndex;
	for (defineIndex = 0; defineIndex < cmd.nDefines; ++defineIndex)
	{
		char *definition = cmd.defines[defineIndex];
		char *separator = strchr(definition, '=');
		if (separator == NULL)
		{
			hashPut(definition, "1", &mcc.hash);
		}
		else
		{
			size_t nameLength = (size_t)(separator - definition);
			char *name = xalloc(nameLength + 1U);
			memcpy(name, definition, nameLength);
			name[nameLength] = '\0';
			hashPut(name, separator + 1, &mcc.hash);
			free(name);
		}
	}
}

static void preprocessSourcePattern(char *pattern)
{
	PLATFORM_FINDDATA fileData;
	PLATFORM_FINDHANDLE findHandle;
	char path[MAX_PATH];
	char *lastSeparator = strrchr(pattern, '/');
	int directoryLength;

	if (lastSeparator == NULL)
	{
		lastSeparator = strrchr(pattern, '\\');
	}
	directoryLength = lastSeparator == NULL ? 0 : (int)(lastSeparator - pattern + 1);
	if (opt & oDEBUG)
	{
		fprintf(stderr, "cc: preprocessing %s\n", pattern);
		fflush(stderr);
	}
	if (platform_findfirst(pattern, &fileData, &findHandle) == -1)
	{
		error("main", "file '%s' not found", pattern);
	}
	do
	{
		int written =
		    snprintf(path, sizeof(path), "%.*s%s", directoryLength, pattern, fileData.name);
		if (written < 0 || (size_t)written >= sizeof(path))
		{
			error("main", "input path is too long");
		}
		prepro(path);
	} while (platform_findnext(&fileData, &findHandle) == 0);
	platform_findclose(&findHandle);
}

static void preprocessInputs(void)
{
	int sourceIndex;
	for (sourceIndex = 0; sourceIndex < cmd.nSrc; ++sourceIndex)
	{
		preprocessSourcePattern(cmd.srcfile[sourceIndex]);
	}
}

static void chooseDefaultOutputFile(void)
{
	char *source;
	char *fileName;
	char *extension;

	if (cmd.outfile[0] != '\0')
	{
		return;
	}
	source = mcc.srcFile[mcc.mainfile];
	fileName = strrchr(source, '/');
	if (fileName == NULL)
	{
		fileName = strrchr(source, '\\');
	}
	copyOption(cmd.outfile,
	           sizeof(cmd.outfile),
	           fileName != NULL ? fileName + 1 : source,
	           "default output file");
	extension = strrchr(cmd.outfile, '.');
	if (extension == NULL)
	{
		extension = cmd.outfile + strlen(cmd.outfile);
	}
	if ((size_t)(extension - cmd.outfile) + 5U > sizeof(cmd.outfile))
	{
		error("main", "default output file name is too long");
	}
	strcpy(extension,
	       (opt & oDLL) ? cmd.target->defaultSharedExtension
	                    : cmd.target->defaultExecutableExtension);
}

static void generateOutput(void)
{
	cmd.target->emitOutput(&compiler, cmd.outfile);
}

int main(int argc, char *argv[])
{
	int emitAssembly;

	initializeCommand();
	emitAssembly = parseCommandLine(argc, argv);
	validateCommandLine(emitAssembly);
	opt |= oUOPT;

	tracePhase("initializing preprocessor");
	ix.tix = -1;
	hashInit('s', 1000, &mcc.hash);
	initPrepro();
	applyCommandLineDefines();
	preprocessInputs();

	mcc.nPreFile = -1;
	tracePhase("adding target startup");
	addStartup();
	if (cmd.jclfile[0] != '\0' && mcc.typeApp != 3)
	{
		error("main", "--zos-jcl requires a translation unit that defines main");
	}
	if (opt & oSRC)
	{
		exit(0);
	}
	if (opt & oLINES)
	{
		printf("%-24s\t%5d\n", "Total", mcc.totalLines);
		exit(0);
	}

	chooseDefaultOutputFile();
	tracePhase("lexing");
	lex();
	tracePhase("initializing instruction table");
	initInstruction();
	hashInit('x', HASHSIZE_TOKEN, &cd.hash);
	tracePhase("parsing");
	parse();
	if (cmd.irfile[0] != '\0')
	{
		FILE *irOutput = fopen(cmd.irfile, "w");
		if (irOutput == NULL)
		{
			error("main", "cannot open IR output file '%s'", cmd.irfile);
		}
		irDumpModule(&compiler.ir, irOutput);
		if (fclose(irOutput) != 0)
		{
			error("main", "cannot close IR output file '%s'", cmd.irfile);
		}
	}
	tracePhase("generating output");
	if (opt & oNAME)
	{
		printNameTable(globTable);
	}
	generateOutput();
	hashFree(&mcc.hash);
	irModuleFree(&compiler.ir);
	free(cmd.libraryPaths);
	return 0;
}

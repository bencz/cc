/*
 * main.c - Ponto de entrada do compilador
 * 
 * Processa argumentos de linha de comando e coordena
 * as fases de compilação.
 */

#include "cc.h"

/*============================================================================
 * Variáveis Globais
 *============================================================================*/

int      opt;
MCC      mcc;
CMDPARAM cmd;
HID      ID;
INDEX    ix;
CODE     cd;
SECTION  mem, raw;
EXE      exe;
Variable var;

/*============================================================================
 * Uso do Programa
 *============================================================================*/

static void printUsage(void)
{
	printf("%s", "Usage: cc [options] file...\n"
		"Options:\n"
		"  -shared    Generate a DLL file (Windows only)\n"
		"  -o <file>  Set output file name\n"
		"  -l<name>   DLL <name>.dll - link with a dll (Windows only)\n"
		"  -hlasm     Generate HLASM output for mainframe\n"
		"  -E         Expression tree\n"
		"  -token     Tokens list\n"
		"  -asm       Generate ASM code listing\n"
		"  --lines    Line count\n");
	exit(0);
}

/*============================================================================
 * Main
 *============================================================================*/

int main(int argc, char *argv[])
{
#ifdef PLATFORM_WINDOWS
	char *impfiles = "msvcrt.dll;kernel32.dll;user32.dll;gdi32.dll;";
#else
	char *impfiles = "";
#endif
	char *opts[] = { "-E", "-shared", "-token", "-asm", "--lines", "-trace", "-debug", "-name", "--opt", "-hlasm" };
	int  flags[] = { oSRC, oDLL, oTOKEN, oASM, oLINES, oTRACE, oDEBUG, oNAME, oUOPT, 0, 0 };
	int  k, n, len;

	mcc.nPreFile = -1;
	strcpy(cmd.impfiles, impfiles);
	len = platform_get_module_path(cmd.MCCDIR, MAX_PATH);
	if (len > 0)
	{
		/* Remove o nome do executável para obter o diretório */
		char *lastSep = strrchr(cmd.MCCDIR, '/');
#ifdef PLATFORM_WINDOWS
		if (!lastSep) lastSep = strrchr(cmd.MCCDIR, '\\');
#endif
		if (lastSep) *lastSep = '\0';
	}
	else
	{
		/* Fallback: usar diretório atual */
		cmd.MCCDIR[0] = '.';
		cmd.MCCDIR[1] = '\0';
	}
	
	for (n = 1; n < argc; n++)
	{
		for (k = 0; flags[k] > 0 && strcmp(argv[n], opts[k]) != 0; k++);
		if (flags[k] > 0) opt |= flags[k];
		else if (strcmp(argv[n], "-hlasm") == 0) g_backend = BACKEND_HLASM;
		else if (strcmp(argv[n], "-o") == 0) strcpy(cmd.outfile, argv[++n]);
		else if (strncmp(argv[n], "-l", 2) == 0)
		{
#ifdef PLATFORM_WINDOWS
			sprintf(cmd.impfiles + strlen(cmd.impfiles), "%s.dll;", argv[n] + 2);
#endif
		}
		else if (argv[n][0] != '-') cmd.srcfile[cmd.nSrc++] = argv[n];
		else error("main", "invalid option -- '%s'", argv[n]);
	}
	if (argc == 1) printUsage();
	if (cmd.nSrc == 0) error("main", "No source files to build");

	opt |= oUOPT;

	ix.tix = -1;
	char path[MAX_PATH], *p, *q;
	initHash('s', 1000, &mcc.hash);
	initPrepro();
	for (n = 0; n < cmd.nSrc; n++)
	{
		PLATFORM_FINDDATA fdata;
		PLATFORM_FINDHANDLE fhandle;
		q = cmd.srcfile[n];
		if (platform_findfirst(q, &fdata, &fhandle) == -1)
			error("mcc.main", "file '%s' not found", q);
		if ((p = strrchr(q, '/')) == NULL) p = strrchr(q, '\\');
		do
		{
			sprintf(path, "%.*s%s", p == NULL ? 0 : (int)(p - q + 1), q, fdata.name);
			prepro(path);
		} while (platform_findnext(&fdata, &fhandle) == 0);
		platform_findclose(&fhandle);
	}
	mcc.nPreFile = -1;
	addStartup();
	if (opt&oSRC) exit(0);
	if (opt&oLINES)
	{
		printf("%-24s\t%5d\n", "Total", mcc.totalLines);
		exit(0);
	}
	if (cmd.outfile[0] == '\0')
	{
		q = mcc.srcFile[mcc.mainfile];
		if ((p = strrchr(q, '/')) == NULL) p = strrchr(q, '\\');
		strcpy(cmd.outfile, p != NULL ? p + 1 : q);
		p = strrchr(cmd.outfile, '.');
		if (g_backend == BACKEND_HLASM)
			strcpy(p, ".asm");
		else
			strcpy(p, (opt&oDLL) ? ".dll" : ".exe");
	}
	freeHash(&mcc.hash);

	lex();

	initInstruction();
	initHash('x', HASHSIZE_TOKEN, &cd.hash);
	parse();
	if (opt&oNAME) printNameTable(globTable);

	/* Seleciona o backend apropriado */
	if (g_backend == BACKEND_HLASM)
	{
#ifdef USE_HLASM_BACKEND
		hlasm_link(cmd.outfile);
#else
		error("main", "HLASM backend not compiled. Recompile with -DUSE_HLASM_BACKEND");
#endif
	}
	else
	{
#ifdef PLATFORM_WINDOWS
		link();
#else
		error("main", "x86/PE backend only available on Windows. Use -hlasm for cross-platform output.");
#endif
	}
	
	return 0;
}

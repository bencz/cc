/*
 * prepro.c - Preprocessador do compilador
 * 
 * Responsável por processar diretivas #include, #define, #pragma
 * e preparar o código fonte para análise léxica.
 */

#include "cc.h"

/*============================================================================
 * Funções Auxiliares de Parsing
 *============================================================================*/

static int parseline(char *p, char *q)
{
	while (*p != '\0' && *p != '\n')
	{
		if (p[0] == '/' && p[1] == '/') break;
		if (p[0] == '/' && p[1] == '*')
		{
			char *r = strstr(p + 2, "*/");
			if (r == NULL)
			{
				*q = '\0';
				return 1;
			}
			strcpy(p, r + 2);
		}
		else if (*p == '\'' || *p == '"')
		{
			char *pEnd = endOfQuote(p);
			while (p < pEnd) *q++ = *p++;
		}
		else
		{
			*q++ = *p++;
		}
	}
	*q = '\0';
	return 0;
}

/*============================================================================
 * Processamento de Diretivas
 *============================================================================*/

static void procInclude(char *src, char *p)
{
	char *q, *r, *s, path[260];

#ifdef PLATFORM_WINDOWS
	const char *sep = "\\";
#else
	const char *sep = "/";
#endif

	if ((q = strchr(p, '<')) != NULL && (r = strchr(q, '>')) != NULL)
	{
		sprintf(path, "%s%sinclude%s%.*s", cmd.MCCDIR, sep, sep, (int)(r - q - 1), q + 1);
	}
	else if ((q = strchr(p, '"')) != NULL && (r = strchr(q + 1, '"')) != NULL)
	{
		if ((s = strrchr(src, '/')) == NULL) s = strrchr(src, '\\');
		if (s == NULL) sprintf(path, "%.*s", (int)(r - q - 1), q + 1);
		else sprintf(path, "%.*s%.*s", (int)(s - src + 1), src, (int)(r - q - 1), q + 1);
	}
	else
	{
		return;
	}
	if (get(path, &mcc.hash) == NULL)
	{
		put(path, p, &mcc.hash);
		prepro(path);
	}
}

static void procDefine(char *str, HASH *pHash)
{
	char *p = strtok(str, " \t");
	char *q = strtok(NULL, "");
	while (*q <= ' ') q++;
	put(p, q, pHash);
}

static void procPragma(char *p)
{
	strtok(p, "\"");
	char *q = strtok(NULL, ".");
	sprintf(cmd.impfiles + strlen(cmd.impfiles), "%s.dll;", q);
}

/*============================================================================
 * Gerenciamento de Linhas de Código
 *============================================================================*/

static void addLine(char *srccode, int nFile, int nLine)
{
	if (mcc.nSrcLine >= mcc.sizeSrcLine)
	{
		mcc.sizeSrcLine = mcc.sizeSrcLine * 3 / 2;
		mcc.pSrcLine = xrealloc(mcc.pSrcLine, mcc.sizeSrcLine*sizeof(SRCLINE));
	}
	SRCLINE *pSL = &mcc.pSrcLine[mcc.nSrcLine++];
	pSL->filenumber = nFile;
	pSL->linenumber = nLine;
	pSL->srccode = xstrdup(srccode);
	if (opt&oSRC) printf("%2d %3d: %s\n", nFile, nLine, srccode);
}

/*============================================================================
 * Preprocessador Principal
 *============================================================================*/

void prepro(char *srcfile)
{
	char buf[512], out[512], key[64], *p, *pBgn, *q;
	int  nLine, nFile = mcc.nSrcFile++;

	FILE *fpSrc = fopen(srcfile, "r");
	if (fpSrc == NULL) error("prepro", "file '%s' not found", srcfile);
	mcc.srcFile[nFile] = xstrdup(srcfile);
	int  fComment = 0;
	for (nLine = 1; fgets(buf, sizeof(buf), fpSrc) != NULL; nLine++)
	{
		mcc.nPreFile = nFile;
		mcc.lines[nFile] = nLine;
		if (fComment)
		{
			if ((p = strstr(buf, "*/")) == NULL) continue;
			strcpy(buf, p + 2);
			fComment = 0;
		}
		fComment = parseline(buf, out);
		for (p = out; *p != '\0' && *p <= ' '; p++);
		if (*p == '\0') continue;
		if (strncmp(out, "#include", 8) == 0)
		{
			procInclude(srcfile, out + 8);
		}
		else if (strncmp(out, "#define", 7) == 0)
		{
			procDefine(out + 7, &mcc.hash);
		}
		else if (strncmp(out, "#pragma", 7) == 0)
		{
			procPragma(out + 7);
		}
		else
		{
			for (p = out; *p != '\0';)
			{
				if (*p == '"' || *p == '\'')
				{
					p = endOfQuote(p);
				}
				else if (!isAlpha(*p))
				{
					if (*p++ == '(' && (strcmp(key, "main") == 0 || strcmp(key, "WinMain") == 0))
					{
						mcc.typeApp = *key == 'm' ? 3 : 2;
						mcc.mainfile = nFile;
					}
				}
				else
				{
					for (q = key, pBgn = p; isAlNum(*p);) *q++ = *p++;
					*q = '\0';
					char *val = get(key, &mcc.hash);
					if (val != NULL)
					{
						int vlen = strlen(val);
						memmove(pBgn + vlen, p, strlen(p) + 1);
						memmove(pBgn, val, vlen);
						p = pBgn;
					}
				}
			}
			addLine(out, nFile, nLine);
		}
	}
	fclose(fpSrc);
	if (opt&oDLL) mcc.typeApp = 0;
	if (opt&oLINES && strstr(srcfile, "include\\") == NULL)
	{
		mcc.totalLines += nLine;
		printf("%-24s\t%5d\n", srcfile, nLine - 1);
	}
}

/*============================================================================
 * Inicialização e Startup
 *============================================================================*/

void initPrepro(void)
{
	mcc.sizeSrcLine = 1000;
	mcc.pSrcLine = xalloc(mcc.sizeSrcLine * sizeof(SRCLINE));
	/* Variáveis de controle FPU x86 - não necessárias para HLASM */
	if (g_backend != BACKEND_HLASM)
	{
		addLine("short _RoundNear = 0x137F;", -1, 1);
		addLine("short _RoundChop = 0x1F7F;", -1, 2);
	}
}

static void addStartupWindows(void)
{
	int n = 3;
	addLine("void _main() {", -1, n++);
	if (mcc.typeApp == 3)  		// CUI
	{
		addLine("  char **argv, **env;", -1, n++);
		addLine("  int argc, new_mode = 0;", -1, n++);
		addLine("  __set_app_type(1);", -1, n++);
		addLine("  _controlfp(0x10000, 0x30000);", -1, n++);
		addLine("  __getmainargs(&argc, &argv, &env, 0, &new_mode);", -1, n++);
		addLine("  exit(main(argc, argv, env));", -1, n++);
	}
	else if (mcc.typeApp == 2)  	// GUI
	{
		addLine("  STARTUPINFOA si;", -1, n++);
		addLine("  __set_app_type(2);", -1, n++);
		addLine("  _controlfp(0x10000, 0x30000);", -1, n++);
		addLine("  char *p = GetCommandLineA();", -1, n++);
		addLine("  int c = (*p++ == '\"') ? '\"' : ' ';", -1, n++);
		addLine("  while (*p != c && *p != 0) p++;", -1, n++);
		addLine("  if (*p != 0) p++;", -1, n++);
		addLine("  while (*p <= ' ' && *p != 0) p++;", -1, n++);
		addLine("  GetStartupInfoA(&si);", -1, n++);
		addLine("  exit(WinMain(GetModuleHandleA((void*)0), (void*)0,", -1, n++);
		addLine("       p, (si.dwFlags&1) ? si.wShowWindow : 10));", -1, n++);
	}
	else  	// DLL
	{
		addLine("  return 1;", -1, n++);
	}
	addLine("}", -1, n++);
}

static void addStartupHLASM(void)
{
	int n = 3;
	/* z/OS startup - chama main() diretamente sem runtime Windows */
	addLine("void _main() {", -1, n++);
	addLine("  int rc;", -1, n++);
	addLine("  rc = main();", -1, n++);
	addLine("}", -1, n++);
}

void addStartup(void)
{
	if (g_backend == BACKEND_HLASM)
	{
		addStartupHLASM();
	}
	else
	{
		addStartupWindows();
	}
}

/*
 * prepro.c - Preprocessador do compilador
 * 
 * Responsável por processar diretivas #include, #define, #pragma
 * e preparar o código fonte para análise léxica.
 */

#include "cc.h"

/*============================================================================
 * Estado de Compilação Condicional
 *============================================================================*/

#define MAX_IF_DEPTH 64

static struct {
	int depth;                    /* profundidade atual do aninhamento */
	int skip[MAX_IF_DEPTH];       /* 1 = pular código neste nível */
	int done[MAX_IF_DEPTH];       /* 1 = já encontrou branch verdadeiro */
} ifstack = {0};

/* Macros predefinidas __DATE__ e __TIME__ */
static char macro_date[16];  /* "Mmm dd yyyy" */
static char macro_time[12];  /* "hh:mm:ss" */

/*============================================================================
 * Macros com Parâmetros
 *============================================================================*/

#define MAX_MACRO_PARAMS 32
#define MAX_MACRO_BODY   4096

typedef struct {
	char name[64];
	char params[MAX_MACRO_PARAMS][64];
	int  nParams;
	int  isVariadic;              /* 1 se último param é ... */
	char body[MAX_MACRO_BODY];
} MacroDef;

static MacroDef macros[256];
static int nMacros = 0;

static MacroDef *findMacro(const char *name)
{
	for (int i = 0; i < nMacros; i++)
		if (strcmp(macros[i].name, name) == 0)
			return &macros[i];
	return NULL;
}

static void deleteMacro(const char *name)
{
	for (int i = 0; i < nMacros; i++)
	{
		if (strcmp(macros[i].name, name) == 0)
		{
			memmove(&macros[i], &macros[i+1], (nMacros - i - 1) * sizeof(MacroDef));
			nMacros--;
			return;
		}
	}
}

/* Retorna 1 se devemos pular o código atual */
static int shouldSkip(void)
{
	for (int i = 0; i <= ifstack.depth; i++)
		if (ifstack.skip[i]) return 1;
	return 0;
}

/*============================================================================
 * Expansão de Macros com Parâmetros
 *============================================================================*/

/* Pula string/char literal e retorna ponteiro para após */
static char *skipStringLiteral(char *p)
{
	char quote = *p++;
	while (*p && *p != quote)
	{
		if (*p == '\\' && p[1]) p++;
		p++;
	}
	if (*p) p++;
	return p;
}

/* Extrai argumentos de uma chamada de macro, retorna número de args */
static int extractMacroArgs(char *start, char *args[], int maxArgs, char **endPtr)
{
	int nArgs = 0;
	int depth = 1;
	char *p = start;
	
	if (*p != '(') return 0;
	p++;
	
	while (*p && *p <= ' ') p++;
	if (*p == ')') { *endPtr = p + 1; return 0; }
	
	char *argStart = p;
	
	while (*p && depth > 0 && nArgs < maxArgs)
	{
		if (*p == '"' || *p == '\'')
		{
			p = skipStringLiteral(p);
		}
		else if (*p == '(')
		{
			depth++;
			p++;
		}
		else if (*p == ')')
		{
			depth--;
			if (depth == 0)
			{
				/* Fim dos argumentos */
				int len = p - argStart;
				args[nArgs] = xalloc(len + 1);
				strncpy(args[nArgs], argStart, len);
				args[nArgs][len] = '\0';
				/* Trim whitespace */
				while (len > 0 && args[nArgs][len-1] <= ' ')
					args[nArgs][--len] = '\0';
				char *s = args[nArgs];
				while (*s && *s <= ' ') s++;
				if (s != args[nArgs]) memmove(args[nArgs], s, strlen(s) + 1);
				nArgs++;
				p++; /* Avança além do ) final */
				break;
			}
			else
			{
				p++;
			}
		}
		else if (*p == ',' && depth == 1)
		{
			/* Separador de argumento */
			int len = p - argStart;
			args[nArgs] = xalloc(len + 1);
			strncpy(args[nArgs], argStart, len);
			args[nArgs][len] = '\0';
			/* Trim whitespace */
			while (len > 0 && args[nArgs][len-1] <= ' ')
				args[nArgs][--len] = '\0';
			char *s = args[nArgs];
			while (*s && *s <= ' ') s++;
			if (s != args[nArgs]) memmove(args[nArgs], s, strlen(s) + 1);
			nArgs++;
			p++;
			while (*p && *p <= ' ') p++;
			argStart = p;
		}
		else
		{
			p++;
		}
	}
	
	*endPtr = p;
	return nArgs;
}

/* Stringify: converte argumento em string literal */
static void stringify(const char *arg, char *out, int maxLen)
{
	int oi = 0;
	out[oi++] = '"';
	for (const char *p = arg; *p && oi < maxLen - 2; p++)
	{
		if (*p == '"' || *p == '\\')
			out[oi++] = '\\';
		out[oi++] = *p;
	}
	out[oi++] = '"';
	out[oi] = '\0';
}

/* Expande uma macro com parâmetros */
static int expandFunctionMacro(MacroDef *m, char *args[], int nArgs, char *out, int maxLen)
{
	char *body = m->body;
	int oi = 0;
	
	while (*body && oi < maxLen - 1)
	{
		/* Operador # (stringify) */
		if (*body == '#' && body[1] != '#')
		{
			body++;
			while (*body && *body <= ' ') body++;
			
			/* Extrai nome do parâmetro */
			char pname[64];
			int pi = 0;
			while (*body && (isAlNum(*body) || *body == '_') && pi < 63)
				pname[pi++] = *body++;
			pname[pi] = '\0';
			
			/* Encontra índice do parâmetro */
			int idx = -1;
			for (int i = 0; i < m->nParams; i++)
			{
				if (strcmp(m->params[i], pname) == 0)
				{
					idx = i;
					break;
				}
			}
			
			if (idx >= 0 && idx < nArgs)
			{
				char tmp[512];
				stringify(args[idx], tmp, sizeof(tmp));
				int len = strlen(tmp);
				if (oi + len < maxLen)
				{
					strcpy(out + oi, tmp);
					oi += len;
				}
			}
			else if (strcmp(pname, "__VA_ARGS__") == 0 && m->isVariadic)
			{
				/* Stringify variadic args */
				char vaargs[2048] = "";
				for (int i = m->nParams; i < nArgs; i++)
				{
					if (i > m->nParams) strcat(vaargs, ", ");
					strcat(vaargs, args[i]);
				}
				char tmp[2048];
				stringify(vaargs, tmp, sizeof(tmp));
				int len = strlen(tmp);
				if (oi + len < maxLen)
				{
					strcpy(out + oi, tmp);
					oi += len;
				}
			}
			continue;
		}
		
		/* Operador ## (concatenação) - remove espaços ao redor */
		if (body[0] == '#' && body[1] == '#')
		{
			/* Remove espaços antes */
			while (oi > 0 && out[oi-1] <= ' ') oi--;
			body += 2;
			/* Pula espaços depois */
			while (*body && *body <= ' ') body++;
			continue;
		}
		
		/* Identificador - pode ser parâmetro ou __VA_ARGS__ */
		if (isAlpha(*body) || *body == '_')
		{
			char pname[64];
			int pi = 0;
			char *pstart = body;
			while (*body && (isAlNum(*body) || *body == '_') && pi < 63)
				pname[pi++] = *body++;
			pname[pi] = '\0';
			
			/* Verifica se é __VA_ARGS__ */
			if (strcmp(pname, "__VA_ARGS__") == 0 && m->isVariadic)
			{
				for (int i = m->nParams; i < nArgs; i++)
				{
					if (i > m->nParams)
					{
						out[oi++] = ',';
						out[oi++] = ' ';
					}
					int len = strlen(args[i]);
					if (oi + len < maxLen)
					{
						strcpy(out + oi, args[i]);
						oi += len;
					}
				}
				continue;
			}
			
			/* Verifica se é parâmetro */
			int idx = -1;
			for (int i = 0; i < m->nParams; i++)
			{
				if (strcmp(m->params[i], pname) == 0)
				{
					idx = i;
					break;
				}
			}
			
			if (idx >= 0 && idx < nArgs)
			{
				int len = strlen(args[idx]);
				if (oi + len < maxLen)
				{
					strcpy(out + oi, args[idx]);
					oi += len;
				}
			}
			else
			{
				/* Não é parâmetro, copia como está */
				int len = strlen(pname);
				if (oi + len < maxLen)
				{
					strcpy(out + oi, pname);
					oi += len;
				}
			}
			continue;
		}
		
		/* Caractere normal */
		out[oi++] = *body++;
	}
	
	out[oi] = '\0';
	return oi;
}

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
	char *p = str;
	while (*p && *p <= ' ') p++;
	
	/* Extrai nome da macro */
	char name[64];
	int ni = 0;
	while (*p && (isAlNum(*p) || *p == '_') && ni < 63)
		name[ni++] = *p++;
	name[ni] = '\0';
	
	/* Verifica se é macro com parâmetros: nome seguido imediatamente por ( */
	if (*p == '(')
	{
		p++; /* pula ( */
		MacroDef *m = &macros[nMacros];
		strcpy(m->name, name);
		m->nParams = 0;
		m->isVariadic = 0;
		
		/* Parse parâmetros */
		while (*p && *p != ')')
		{
			while (*p && *p <= ' ') p++;
			if (*p == ')') break;
			
			/* Verifica ... (variadic) */
			if (strncmp(p, "...", 3) == 0)
			{
				m->isVariadic = 1;
				p += 3;
				while (*p && *p <= ' ') p++;
				break;
			}
			
			/* Extrai nome do parâmetro */
			int pi = 0;
			while (*p && (isAlNum(*p) || *p == '_') && pi < 63)
				m->params[m->nParams][pi++] = *p++;
			m->params[m->nParams][pi] = '\0';
			
			/* Verifica se param é seguido por ... */
			while (*p && *p <= ' ') p++;
			if (strncmp(p, "...", 3) == 0)
			{
				m->isVariadic = 1;
				p += 3;
				while (*p && *p <= ' ') p++;
			}
			
			m->nParams++;
			while (*p && *p <= ' ') p++;
			if (*p == ',') p++;
		}
		if (*p == ')') p++;
		
		/* Pula espaços antes do corpo */
		while (*p && *p <= ' ') p++;
		
		/* Copia corpo da macro */
		strncpy(m->body, p, MAX_MACRO_BODY - 1);
		m->body[MAX_MACRO_BODY - 1] = '\0';
		
		/* Remove trailing whitespace */
		int len = strlen(m->body);
		while (len > 0 && m->body[len-1] <= ' ')
			m->body[--len] = '\0';
		
		nMacros++;
		
		/* Também registra na hash para que defined() funcione */
		put(name, "", pHash);
	}
	else
	{
		/* Macro simples sem parâmetros */
		while (*p && *p <= ' ') p++;
		if (*p == '\0')
			p = "";
		put(name, p, pHash);
	}
}

static void procUndef(char *str, HASH *pHash)
{
	char *p = str;
	while (*p && *p <= ' ') p++;
	char name[64];
	int ni = 0;
	while (*p && (isAlNum(*p) || *p == '_') && ni < 63)
		name[ni++] = *p++;
	name[ni] = '\0';
	
	if (name[0] != '\0')
	{
		del(name, pHash);
		deleteMacro(name);
	}
}

static void procIfdef(char *str, HASH *pHash, int negate)
{
	if (ifstack.depth >= MAX_IF_DEPTH - 1)
		error("prepro", "#if nesting too deep");
	ifstack.depth++;
	char *p = strtok(str, " \t");
	int defined = (p != NULL && get(p, pHash) != NULL);
	if (negate) defined = !defined;
	ifstack.skip[ifstack.depth] = !defined;
	ifstack.done[ifstack.depth] = defined;
}

static void procElse(void)
{
	if (ifstack.depth <= 0)
		error("prepro", "#else without #if");
	if (ifstack.done[ifstack.depth])
		ifstack.skip[ifstack.depth] = 1;
	else
	{
		ifstack.skip[ifstack.depth] = 0;
		ifstack.done[ifstack.depth] = 1;
	}
}

static void procElif(char *str, HASH *pHash)
{
	if (ifstack.depth <= 0)
		error("prepro", "#elif without #if");
	if (ifstack.done[ifstack.depth])
	{
		ifstack.skip[ifstack.depth] = 1;
	}
	else
	{
		/* Avalia expressão simples: defined(X) ou apenas X */
		char *p = str;
		while (*p && *p <= ' ') p++;
		int result = 0;
		if (strncmp(p, "defined", 7) == 0)
		{
			p += 7;
			while (*p && (*p <= ' ' || *p == '(')) p++;
			char *end = p;
			while (*end && *end != ')' && *end > ' ') end++;
			char save = *end;
			*end = '\0';
			result = (get(p, pHash) != NULL);
			*end = save;
		}
		else
		{
			char *name = strtok(p, " \t");
			if (name != NULL)
				result = (get(name, pHash) != NULL);
		}
		ifstack.skip[ifstack.depth] = !result;
		ifstack.done[ifstack.depth] = result;
	}
}

static void procEndif(void)
{
	if (ifstack.depth <= 0)
		error("prepro", "#endif without #if");
	ifstack.skip[ifstack.depth] = 0;
	ifstack.done[ifstack.depth] = 0;
	ifstack.depth--;
}

static void procIf(char *str, HASH *pHash)
{
	if (ifstack.depth >= MAX_IF_DEPTH - 1)
		error("prepro", "#if nesting too deep");
	ifstack.depth++;
	
	char *p = str;
	while (*p && *p <= ' ') p++;
	
	int result = 0;
	int negate = 0;
	
	/* Suporta !defined(X) e defined(X) */
	if (*p == '!')
	{
		negate = 1;
		p++;
		while (*p && *p <= ' ') p++;
	}
	
	if (strncmp(p, "defined", 7) == 0)
	{
		p += 7;
		while (*p && (*p <= ' ' || *p == '(')) p++;
		char *end = p;
		while (*end && *end != ')' && *end > ' ') end++;
		char save = *end;
		*end = '\0';
		result = (get(p, pHash) != NULL);
		*end = save;
	}
	else
	{
		/* Avalia como número: 0 = false, != 0 = true */
		result = (atoi(p) != 0);
	}
	
	if (negate) result = !result;
	ifstack.skip[ifstack.depth] = !result;
	ifstack.done[ifstack.depth] = result;
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
		/* Diretivas condicionais - sempre processadas */
		if (strncmp(out, "#ifdef", 6) == 0)
		{
			procIfdef(out + 6, &mcc.hash, 0);
		}
		else if (strncmp(out, "#ifndef", 7) == 0)
		{
			procIfdef(out + 7, &mcc.hash, 1);
		}
		else if (strncmp(out, "#if", 3) == 0 && (out[3] == ' ' || out[3] == '\t'))
		{
			procIf(out + 3, &mcc.hash);
		}
		else if (strncmp(out, "#elif", 5) == 0)
		{
			procElif(out + 5, &mcc.hash);
		}
		else if (strncmp(out, "#else", 5) == 0)
		{
			procElse();
		}
		else if (strncmp(out, "#endif", 6) == 0)
		{
			procEndif();
		}
		else if (shouldSkip())
		{
			/* Pula código dentro de bloco condicional falso */
			continue;
		}
		else if (strncmp(out, "#include", 8) == 0)
		{
			procInclude(srcfile, out + 8);
		}
		else if (strncmp(out, "#define", 7) == 0)
		{
			procDefine(out + 7, &mcc.hash);
		}
		else if (strncmp(out, "#undef", 6) == 0)
		{
			procUndef(out + 6, &mcc.hash);
		}
		else if (strncmp(out, "#pragma", 7) == 0)
		{
			procPragma(out + 7);
		}
		else if (strncmp(out, "#error", 6) == 0)
		{
			error("prepro", "%s", out + 6);
		}
		else if (strncmp(out, "#warning", 8) == 0)
		{
			fprintf(stderr, "warning: %s\n", out + 8);
		}
		else if (strncmp(out, "#line", 5) == 0)
		{
			/* #line NUMBER ["FILENAME"] */
			char *lp = out + 5;
			while (*lp && *lp <= ' ') lp++;
			int newLine = atoi(lp);
			if (newLine > 0) nLine = newLine - 1;
			while (*lp && *lp > ' ' && *lp != '"') lp++;
			while (*lp && *lp <= ' ') lp++;
			if (*lp == '"')
			{
				lp++;
				char *end = strchr(lp, '"');
				if (end)
				{
					*end = '\0';
					free(mcc.srcFile[nFile]);
					mcc.srcFile[nFile] = xstrdup(lp);
				}
			}
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
					char *val = NULL;
					char tmpval[4096];
					int freeArgs = 0;
					char *args[MAX_MACRO_PARAMS];
					int nArgs = 0;
					
					/* Macros predefinidas ISO C99 */
					if (strcmp(key, "__FILE__") == 0)
					{
						sprintf(tmpval, "\"%s\"", srcfile);
						val = tmpval;
					}
					else if (strcmp(key, "__LINE__") == 0)
					{
						sprintf(tmpval, "%d", nLine);
						val = tmpval;
					}
					else if (strcmp(key, "__STDC__") == 0)
					{
						val = "1";
					}
					else if (strcmp(key, "__STDC_VERSION__") == 0)
					{
						val = "199901L";
					}
					else if (strcmp(key, "__DATE__") == 0)
					{
						val = macro_date;
					}
					else if (strcmp(key, "__TIME__") == 0)
					{
						val = macro_time;
					}
					else
					{
						/* Verifica se é macro com parâmetros */
						MacroDef *m = findMacro(key);
						if (m != NULL && *p == '(')
						{
							char *endArgs;
							nArgs = extractMacroArgs(p, args, MAX_MACRO_PARAMS, &endArgs);
							freeArgs = 1;
							expandFunctionMacro(m, args, nArgs, tmpval, sizeof(tmpval));
							val = tmpval;
							p = endArgs;
						}
						else
						{
							val = get(key, &mcc.hash);
						}
					}
					if (val != NULL)
					{
						int vlen = strlen(val);
						memmove(pBgn + vlen, p, strlen(p) + 1);
						memmove(pBgn, val, vlen);
						p = pBgn;
					}
					/* Libera argumentos alocados */
					if (freeArgs)
					{
						for (int i = 0; i < nArgs; i++)
							free(args[i]);
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
	
	/* Inicializa macros __DATE__ e __TIME__ */
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	static const char *months[] = {"Jan","Feb","Mar","Apr","May","Jun",
	                               "Jul","Aug","Sep","Oct","Nov","Dec"};
	sprintf(macro_date, "\"%s %2d %d\"", months[tm->tm_mon], tm->tm_mday, tm->tm_year + 1900);
	sprintf(macro_time, "\"%02d:%02d:%02d\"", tm->tm_hour, tm->tm_min, tm->tm_sec);
	
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

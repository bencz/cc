/* Preprocessor directives, macro expansion, and logical source lines. */

#include "cc.h"

/* Conditional-compilation state */

#define MAX_IF_DEPTH 64

typedef struct _ConditionalStack
{
	int depth;
	int skip[MAX_IF_DEPTH];
	int branchTaken[MAX_IF_DEPTH];
	int elseSeen[MAX_IF_DEPTH];
} ConditionalStack;

static ConditionalStack conditionalStack = {0};

/* Predefined __DATE__ and __TIME__ values. */
static char macro_date[16]; /* "Mmm dd yyyy" */
static char macro_time[12]; /* "hh:mm:ss" */

/* Function-like macros */

#define MAX_MACRO_PARAMS 32
#define MAX_MACRO_BODY 4096

typedef struct _MacroDef
{
	char name[64];
	char params[MAX_MACRO_PARAMS][64];
	int parameterCount;
	int variadic;
	char body[MAX_MACRO_BODY];
} MacroDef;

static MacroDef macros[256];
static int macroCount = 0;

static MacroDef *findMacro(const char *name)
{
	for (int i = 0; i < macroCount; i++)
	{
		if (strcmp(macros[i].name, name) == 0)
		{
			return &macros[i];
		}
	}
	return NULL;
}

static void deleteMacro(const char *name)
{
	for (int i = 0; i < macroCount; i++)
	{
		if (strcmp(macros[i].name, name) == 0)
		{
			memmove(&macros[i], &macros[i + 1], (macroCount - i - 1) * sizeof(MacroDef));
			macroCount--;
			return;
		}
	}
}

static int shouldSkip(void)
{
	for (int i = 0; i <= conditionalStack.depth; i++)
	{
		if (conditionalStack.skip[i])
		{
			return 1;
		}
	}
	return 0;
}

static int parentShouldSkip(void)
{
	for (int i = 0; i < conditionalStack.depth; ++i)
	{
		if (conditionalStack.skip[i])
		{
			return 1;
		}
	}
	return 0;
}

/* Function-like macro expansion */

static char *skipStringLiteral(char *p)
{
	char quote = *p++;
	while (*p && *p != quote)
	{
		if (*p == '\\' && p[1])
		{
			p++;
		}
		p++;
	}
	if (*p)
	{
		p++;
	}
	return p;
}

static int extractMacroArgs(char *start, char *args[], int maxArgs, char **endPtr)
{
	int nArgs = 0;
	int depth = 1;
	char *p = start;

	if (*p != '(')
	{
		return 0;
	}
	p++;

	while (*p && *p <= ' ')
	{
		p++;
	}
	if (*p == ')')
	{
		*endPtr = p + 1;
		return 0;
	}

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
				/* Final argument */
				int len = p - argStart;
				args[nArgs] = xalloc(len + 1);
				strncpy(args[nArgs], argStart, len);
				args[nArgs][len] = '\0';
				/* Trim whitespace */
				while (len > 0 && args[nArgs][len - 1] <= ' ')
				{
					args[nArgs][--len] = '\0';
				}
				char *s = args[nArgs];
				while (*s && *s <= ' ')
				{
					s++;
				}
				if (s != args[nArgs])
				{
					memmove(args[nArgs], s, strlen(s) + 1);
				}
				nArgs++;
				p++; /* Consume the closing parenthesis. */
				break;
			}
			else
			{
				p++;
			}
		}
		else if (*p == ',' && depth == 1)
		{
			/* Argument separator */
			int len = p - argStart;
			args[nArgs] = xalloc(len + 1);
			strncpy(args[nArgs], argStart, len);
			args[nArgs][len] = '\0';
			/* Trim whitespace */
			while (len > 0 && args[nArgs][len - 1] <= ' ')
			{
				args[nArgs][--len] = '\0';
			}
			char *s = args[nArgs];
			while (*s && *s <= ' ')
			{
				s++;
			}
			if (s != args[nArgs])
			{
				memmove(args[nArgs], s, strlen(s) + 1);
			}
			nArgs++;
			p++;
			while (*p && *p <= ' ')
			{
				p++;
			}
			argStart = p;
		}
		else
		{
			p++;
		}
	}
	if (depth > 0)
	{
		error("prepro",
		      nArgs >= maxArgs ? "too many macro arguments" : "unterminated macro invocation");
	}

	*endPtr = p;
	return nArgs;
}

/* Convert one macro argument to a string literal. */
static void stringify(const char *arg, char *out, int maxLen)
{
	int oi = 0;
	if (maxLen < 3)
	{
		error("prepro", "stringification buffer is too small");
	}
	out[oi++] = '"';
	for (const char *p = arg; *p; p++)
	{
		if (*p == '"' || *p == '\\')
		{
			if (oi + 2 >= maxLen)
			{
				error("prepro", "stringified macro argument is too large");
			}
			out[oi++] = '\\';
		}
		else if (oi + 1 >= maxLen)
		{
			error("prepro", "stringified macro argument is too large");
		}
		out[oi++] = *p;
	}
	if (oi + 1 >= maxLen)
	{
		error("prepro", "stringified macro argument is too large");
	}
	out[oi++] = '"';
	out[oi] = '\0';
}

/* Expand a function-like macro. */
static int expandFunctionMacro(MacroDef *m, char *args[], int nArgs, char *out, int maxLen)
{
	char *body = m->body;
	int oi = 0;

	while (*body && oi < maxLen - 1)
	{
		if (*body == '"' || *body == '\'')
		{
			char quote = *body++;
			out[oi++] = quote;
			while (*body != '\0')
			{
				if (oi >= maxLen - 1)
				{
					error("prepro", "macro expansion is too large");
				}
				out[oi++] = *body;
				if (*body == '\\' && body[1] != '\0')
				{
					++body;
					if (oi >= maxLen - 1)
					{
						error("prepro", "macro expansion is too large");
					}
					out[oi++] = *body;
				}
				else if (*body == quote)
				{
					++body;
					break;
				}
				++body;
			}
			continue;
		}
		/* Stringification */
		if (*body == '#' && body[1] != '#')
		{
			body++;
			while (*body && *body <= ' ')
			{
				body++;
			}

			/* Parameter name */
			char pname[64];
			int pi = 0;
			while (*body && (isIdentifierContinue(*body) || *body == '_') && pi < 63)
			{
				pname[pi++] = *body++;
			}
			pname[pi] = '\0';

			/* Parameter index */
			int idx = -1;
			for (int i = 0; i < m->parameterCount; i++)
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
				if (oi + len >= maxLen)
				{
					error("prepro", "macro expansion is too large");
				}
				strcpy(out + oi, tmp);
				oi += len;
			}
			else if (strcmp(pname, "__VA_ARGS__") == 0 && m->variadic)
			{
				/* Stringify the complete variadic argument list. */
				char vaargs[2048] = "";
				for (int i = m->parameterCount; i < nArgs; i++)
				{
					size_t used = strlen(vaargs);
					size_t argumentLength = strlen(args[i]);
					size_t separatorLength = i > m->parameterCount ? 2U : 0U;
					if (used + separatorLength + argumentLength + 1U > sizeof(vaargs))
					{
						error("prepro", "variadic macro argument list is too large");
					}
					if (separatorLength != 0U)
					{
						memcpy(vaargs + used, ", ", 2U), used += 2U;
					}
					memcpy(vaargs + used, args[i], argumentLength + 1U);
				}
				char tmp[2048];
				stringify(vaargs, tmp, sizeof(tmp));
				int len = strlen(tmp);
				if (oi + len >= maxLen)
				{
					error("prepro", "macro expansion is too large");
				}
				strcpy(out + oi, tmp);
				oi += len;
			}
			continue;
		}

		/* Token pasting */
		if (body[0] == '#' && body[1] == '#')
		{
			/* Remove leading whitespace. */
			while (oi > 0 && out[oi - 1] <= ' ')
			{
				oi--;
			}
			body += 2;
			/* Skip trailing whitespace. */
			while (*body && *body <= ' ')
			{
				body++;
			}
			continue;
		}

		/* Parameter or __VA_ARGS__ substitution. */
		if (isIdentifierStart(*body) || *body == '_')
		{
			char pname[64];
			int pi = 0;
			while (*body && (isIdentifierContinue(*body) || *body == '_') && pi < 63)
			{
				pname[pi++] = *body++;
			}
			pname[pi] = '\0';

			/* Variadic substitution */
			if (strcmp(pname, "__VA_ARGS__") == 0 && m->variadic)
			{
				for (int i = m->parameterCount; i < nArgs; i++)
				{
					int len = strlen(args[i]);
					if (i > m->parameterCount)
					{
						if (oi + 2 + len >= maxLen)
						{
							error("prepro", "macro expansion is too large");
						}
						out[oi++] = ',';
						out[oi++] = ' ';
					}
					if (oi + len >= maxLen)
					{
						error("prepro", "macro expansion is too large");
					}
					strcpy(out + oi, args[i]);
					oi += len;
				}
				continue;
			}

			/* Named parameter substitution */
			int idx = -1;
			for (int i = 0; i < m->parameterCount; i++)
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
				if (oi + len >= maxLen)
				{
					error("prepro", "macro expansion is too large");
				}
				strcpy(out + oi, args[idx]);
				oi += len;
			}
			else
			{
				/* Preserve ordinary identifiers. */
				int len = strlen(pname);
				if (oi + len >= maxLen)
				{
					error("prepro", "macro expansion is too large");
				}
				strcpy(out + oi, pname);
				oi += len;
			}
			continue;
		}

		/* Ordinary character */
		out[oi++] = *body++;
	}

	if (*body != '\0')
	{
		error("prepro", "macro expansion is too large");
	}
	out[oi] = '\0';
	return oi;
}

int preproExpandFunctionExpression(const char *name, const char **cursor, char **replacement)
{
	MacroDef *macro = findMacro(name);
	char *arguments[MAX_MACRO_PARAMS];
	char *invocation = (char *)*cursor;
	char *end;
	char *expanded;
	int argumentCount;
	int index;
	if (macro == NULL)
	{
		return 0;
	}
	while (*invocation == ' ' || *invocation == '\t')
	{
		++invocation;
	}
	if (*invocation != '(')
	{
		return 0;
	}
	argumentCount = extractMacroArgs(invocation, arguments, MAX_MACRO_PARAMS, &end);
	if (argumentCount == 0 && macro->parameterCount == 1)
	{
		arguments[argumentCount++] = xstrdup("");
	}
	if ((!macro->variadic && argumentCount != macro->parameterCount) ||
	    (macro->variadic && argumentCount < macro->parameterCount))
	{
		error("prepro", "macro '%s' has the wrong number of arguments in #if", name);
	}
	expanded = xalloc(MAX_MACRO_BODY);
	expandFunctionMacro(macro, arguments, argumentCount, expanded, MAX_MACRO_BODY);
	for (index = 0; index < argumentCount; ++index)
	{
		free(arguments[index]);
	}
	*cursor = end;
	*replacement = expanded;
	return 1;
}

/* Text parsing */

static int parseline(char *p, char *q)
{
	while (*p != '\0' && *p != '\n')
	{
		if (p[0] == '/' && p[1] == '/')
		{
			break;
		}
		if (p[0] == '/' && p[1] == '*')
		{
			char *r = strstr(p + 2, "*/");
			if (r == NULL)
			{
				*q++ = ' ';
				*q = '\0';
				return 1;
			}
			*q++ = ' ';
			p = r + 2;
		}
		else if (*p == '\'' || *p == '"')
		{
			char *pEnd = skipQuotedLiteral(p);
			while (p < pEnd)
			{
				*q++ = *p++;
			}
		}
		else
		{
			*q++ = *p++;
		}
	}
	*q = '\0';
	return 0;
}

/* Read one C logical source line.  Backslash-newline splicing happens before
 * comment removal and directive recognition, as required by the translation
 * phase ordering.  The returned buffer is owned by the caller. */
static char *readLogicalLine(FILE *input, int *nextLine, int *startLine)
{
	size_t capacity = 256U;
	size_t length = 0U;
	char *line = xalloc(capacity);
	int c;
	int sawAny = 0;
	*startLine = *nextLine;
	for (;;)
	{
		c = fgetc(input);
		if (c == EOF)
		{
			if (ferror(input))
			{
				error("prepro", "source read error");
			}
			break;
		}
		sawAny = 1;
		if (c == '\r')
		{
			int following = fgetc(input);
			if (following != '\n' && following != EOF)
			{
				ungetc(following, input);
			}
			c = '\n';
		}
		if (c == '\n')
		{
			++*nextLine;
			if (length > 0U && line[length - 1U] == '\\')
			{
				--length;
				continue;
			}
			break;
		}
		if (length + 1U >= capacity)
		{
			if (capacity > SIZE_MAX / 2U)
			{
				error("prepro", "logical source line is too large");
			}
			capacity *= 2U;
			line = xrealloc(line, capacity);
		}
		line[length++] = (char)c;
	}
	if (!sawAny && length == 0U)
	{
		free(line);
		return NULL;
	}
	line[length] = '\0';
	return line;
}

/* Directive handling */

static void processInclude(char *src, char *p)
{
	char *q, *r, *s, path[260];
	int written;

	const char *sep = "/";

	if ((q = strchr(p, '<')) != NULL && (r = strchr(q, '>')) != NULL)
	{
		written = snprintf(
		    path, sizeof(path), "%s%sinclude%s%.*s", cmd.MCCDIR, sep, sep, (int)(r - q - 1), q + 1);
	}
	else if ((q = strchr(p, '"')) != NULL && (r = strchr(q + 1, '"')) != NULL)
	{
		if ((s = strrchr(src, '/')) == NULL)
		{
			s = strrchr(src, '\\');
		}
		if (s == NULL)
		{
			written = snprintf(path, sizeof(path), "%.*s", (int)(r - q - 1), q + 1);
		}
		else
		{
			written = snprintf(
			    path, sizeof(path), "%.*s%.*s", (int)(s - src + 1), src, (int)(r - q - 1), q + 1);
		}
	}
	else
	{
		error("prepro", "malformed #include directive");
		return;
	}
	if (written < 0 || (size_t)written >= sizeof(path))
	{
		error("prepro", "included file path is too long");
	}
	prepro(path);
}

static void processDefine(char *str, HASH *pHash)
{
	char *p = str;
	while (*p && *p <= ' ')
	{
		p++;
	}

	/* Macro name */
	char name[64];
	int ni = 0;
	while (*p && (isIdentifierContinue(*p) || *p == '_') && ni < 63)
	{
		name[ni++] = *p++;
	}
	name[ni] = '\0';
	if (name[0] == '\0' || !isIdentifierStart((unsigned char)name[0]))
	{
		error("prepro", "macro name expected after #define");
	}
	if (isIdentifierContinue((unsigned char)*p) || *p == '_')
	{
		error("prepro", "macro name exceeds %u characters", (unsigned int)sizeof(name) - 1U);
	}

	/* A function-like macro requires '(' immediately after its name. */
	if (*p == '(')
	{
		p++; /* Consume '('. */
		MacroDef *m = findMacro(name);
		if (m == NULL)
		{
			if (macroCount >= (int)(sizeof(macros) / sizeof(macros[0])))
			{
				error("prepro", "too many function-like macros");
			}
			m = &macros[macroCount++];
		}
		memset(m, 0, sizeof(*m));
		strcpy(m->name, name);
		m->parameterCount = 0;
		m->variadic = 0;

		/* Parameter list */
		while (*p && *p != ')')
		{
			while (*p && *p <= ' ')
			{
				p++;
			}
			if (*p == ')')
			{
				break;
			}

			/* Unnamed variadic parameter */
			if (strncmp(p, "...", 3) == 0)
			{
				m->variadic = 1;
				p += 3;
				while (*p && *p <= ' ')
				{
					p++;
				}
				break;
			}

			/* Parameter name */
			int pi = 0;
			if (m->parameterCount >= MAX_MACRO_PARAMS)
			{
				error("prepro", "too many parameters in macro '%s'", name);
			}
			while (*p && (isIdentifierContinue(*p) || *p == '_') && pi < 63)
			{
				m->params[m->parameterCount][pi++] = *p++;
			}
			m->params[m->parameterCount][pi] = '\0';
			if (pi == 0 || !isIdentifierStart((unsigned char)m->params[m->parameterCount][0]))
			{
				error("prepro", "invalid parameter in macro '%s'", name);
			}
			if (isIdentifierContinue((unsigned char)*p) || *p == '_')
			{
				error("prepro", "parameter name in macro '%s' is too long", name);
			}
			for (int parameter = 0; parameter < m->parameterCount; ++parameter)
			{
				if (strcmp(m->params[parameter], m->params[m->parameterCount]) == 0)
				{
					error("prepro",
					      "duplicate parameter '%s' in macro '%s'",
					      m->params[m->parameterCount],
					      name);
				}
			}

			/* Named variadic parameter */
			while (*p && *p <= ' ')
			{
				p++;
			}
			if (strncmp(p, "...", 3) == 0)
			{
				m->variadic = 1;
				p += 3;
				while (*p && *p <= ' ')
				{
					p++;
				}
			}

			m->parameterCount++;
			while (*p && *p <= ' ')
			{
				p++;
			}
			if (*p == ',')
			{
				p++;
			}
			else if (*p != ')')
			{
				error("prepro", "',' or ')' expected in macro '%s'", name);
			}
		}
		if (*p != ')')
		{
			error("prepro", "unterminated parameter list in macro '%s'", name);
		}
		p++;

		/* Skip whitespace before the replacement list. */
		while (*p && *p <= ' ')
		{
			p++;
		}

		/* Copy the replacement list. */
		if (strlen(p) >= sizeof(m->body))
		{
			error("prepro", "replacement list for macro '%s' is too large", name);
		}
		strcpy(m->body, p);

		/* Remove trailing whitespace */
		int len = strlen(m->body);
		while (len > 0 && m->body[len - 1] <= ' ')
		{
			m->body[--len] = '\0';
		}

		/* Keep function-like macros visible to defined(). */
		hashPut(name, "", pHash);
	}
	else
	{
		deleteMacro(name);
		/* Object-like macro */
		while (*p && *p <= ' ')
		{
			p++;
		}
		if (*p == '\0')
		{
			p = "";
		}
		hashPut(name, p, pHash);
	}
}

static void processUndef(char *str, HASH *pHash)
{
	char *p = str;
	while (*p && *p <= ' ')
	{
		p++;
	}
	char name[64];
	int ni = 0;
	while (*p && (isIdentifierContinue(*p) || *p == '_') && ni < 63)
	{
		name[ni++] = *p++;
	}
	name[ni] = '\0';

	if (name[0] != '\0')
	{
		hashRemove(name, pHash);
		deleteMacro(name);
	}
}

static void processIfdef(char *str, HASH *pHash, int negate)
{
	int parentSkipping;
	if (conditionalStack.depth >= MAX_IF_DEPTH - 1)
	{
		error("prepro", "#if nesting too deep");
	}
	parentSkipping = shouldSkip();
	conditionalStack.depth++;
	char *p = strtok(str, " \t");
	int defined = !parentSkipping && (p != NULL && hashGet(p, pHash) != NULL);
	if (negate)
	{
		defined = !defined;
	}
	if (parentSkipping)
	{
		defined = 0;
	}
	conditionalStack.skip[conditionalStack.depth] = !defined;
	conditionalStack.branchTaken[conditionalStack.depth] = defined;
	conditionalStack.elseSeen[conditionalStack.depth] = 0;
}

static void processElse(void)
{
	if (conditionalStack.depth <= 0)
	{
		error("prepro", "#else without #if");
	}
	if (conditionalStack.elseSeen[conditionalStack.depth])
	{
		error("prepro", "duplicate #else");
	}
	conditionalStack.elseSeen[conditionalStack.depth] = 1;
	if (conditionalStack.branchTaken[conditionalStack.depth])
	{
		conditionalStack.skip[conditionalStack.depth] = 1;
	}
	else
	{
		conditionalStack.skip[conditionalStack.depth] = 0;
		conditionalStack.branchTaken[conditionalStack.depth] = 1;
	}
}

static void processEndif(void)
{
	if (conditionalStack.depth <= 0)
	{
		error("prepro", "#endif without #if");
	}
	conditionalStack.skip[conditionalStack.depth] = 0;
	conditionalStack.branchTaken[conditionalStack.depth] = 0;
	conditionalStack.elseSeen[conditionalStack.depth] = 0;
	--conditionalStack.depth;
}

static void processElif(char *str, HASH *pHash)
{
	int result;
	if (conditionalStack.depth <= 0)
	{
		error("prepro", "#elif without #if");
	}
	if (conditionalStack.elseSeen[conditionalStack.depth])
	{
		error("prepro", "#elif after #else");
	}
	if (parentShouldSkip())
	{
		conditionalStack.skip[conditionalStack.depth] = 1;
		return;
	}
	if (conditionalStack.branchTaken[conditionalStack.depth])
	{
		conditionalStack.skip[conditionalStack.depth] = 1;
		return;
	}
	result = preproEvalExpression(str, pHash);
	conditionalStack.skip[conditionalStack.depth] = !result;
	conditionalStack.branchTaken[conditionalStack.depth] = result;
}

static void processIf(char *str, HASH *pHash)
{
	int result;
	int parentSkipping;
	if (conditionalStack.depth >= MAX_IF_DEPTH - 1)
	{
		error("prepro", "#if nesting too deep");
	}
	parentSkipping = shouldSkip();
	result = parentSkipping ? 0 : preproEvalExpression(str, pHash);
	++conditionalStack.depth;
	conditionalStack.skip[conditionalStack.depth] = !result;
	conditionalStack.branchTaken[conditionalStack.depth] = result;
	conditionalStack.elseSeen[conditionalStack.depth] = 0;
}

static void processPragma(char *p)
{
	char *begin;
	char *end;
	size_t used;
	size_t nameLength;
	while (*p == ' ' || *p == '\t')
	{
		++p;
	}
	if (strncmp(p, "comment", 7) != 0)
	{
		return;
	}
	begin = strchr(p + 7, '"');
	if (begin == NULL)
	{
		error("prepro", "malformed #pragma comment");
	}
	end = strchr(++begin, '"');
	if (end == NULL)
	{
		error("prepro", "malformed #pragma comment library name");
	}
	nameLength = (size_t)(end - begin);
	used = strlen(cmd.impfiles);
	if (used + nameLength + 6U > sizeof(cmd.impfiles))
	{
		error("prepro", "import library list is too large");
	}
	memcpy(cmd.impfiles + used, begin, nameLength);
	memcpy(cmd.impfiles + used + nameLength, ".dll;", 6U);
}

/* Source-line storage */

static void addLine(const char *srccode, int nFile, int nLine)
{
	if (mcc.nSrcLine >= mcc.sizeSrcLine)
	{
		mcc.sizeSrcLine = mcc.sizeSrcLine * 3 / 2;
		mcc.pSrcLine = xrealloc(mcc.pSrcLine, mcc.sizeSrcLine * sizeof(SRCLINE));
	}
	SRCLINE *pSL = &mcc.pSrcLine[mcc.nSrcLine++];
	pSL->filenumber = nFile;
	pSL->linenumber = nLine;
	pSL->srccode = xstrdup(srccode);
	if (opt & oSRC)
	{
		printf("%2d %3d: %s\n", nFile, nLine, srccode);
	}
}

void preproAddSyntheticLine(const char *source, int line)
{
	addLine(source, -1, line);
}

/* Main preprocessing pass */

void prepro(char *srcfile)
{
	char key[64], *p, *pBgn, *q;
	int nLine;
	int nextLine = 1;
	int initialIfDepth = conditionalStack.depth;
	int nFile;
	key[0] = '\0';
	if (mcc.nSrcFile >= (int)(sizeof(mcc.srcFile) / sizeof(mcc.srcFile[0])))
	{
		error("prepro", "too many source and include files");
	}
	nFile = mcc.nSrcFile++;

	FILE *fpSrc = fopen(srcfile, "r");
	if (fpSrc == NULL)
	{
		error("prepro", "file '%s' not found", srcfile);
	}
	mcc.srcFile[nFile] = xstrdup(srcfile);
	int fComment = 0;
	for (;;)
	{
		char *buf = readLogicalLine(fpSrc, &nextLine, &nLine);
		char *out;
		size_t outCapacity;
		int expansionCount = 0;
		if (buf == NULL)
		{
			break;
		}
		outCapacity = strlen(buf) + 1U;
		out = xalloc(outCapacity);
		mcc.nPreFile = nFile;
		mcc.lines[nFile] = nLine;
		if (fComment)
		{
			if ((p = strstr(buf, "*/")) == NULL)
			{
				free(out);
				free(buf);
				continue;
			}
			memmove(buf, p + 2, strlen(p + 2) + 1U);
			fComment = 0;
		}
		fComment = parseline(buf, out);
		for (p = out; *p != '\0' && *p <= ' '; p++)
			;
		if (*p == '\0')
		{
			free(out);
			free(buf);
			continue;
		}
		/* Conditional directives remain active inside skipped regions. */
		if (strncmp(p, "#ifdef", 6) == 0 && (p[6] == '\0' || p[6] <= ' '))
		{
			processIfdef(p + 6, &mcc.hash, 0);
		}
		else if (strncmp(p, "#ifndef", 7) == 0 && (p[7] == '\0' || p[7] <= ' '))
		{
			processIfdef(p + 7, &mcc.hash, 1);
		}
		else if (strncmp(p, "#if", 3) == 0 && (p[3] == '\0' || p[3] <= ' '))
		{
			processIf(p + 3, &mcc.hash);
		}
		else if (strncmp(p, "#elif", 5) == 0 && (p[5] == '\0' || p[5] <= ' '))
		{
			processElif(p + 5, &mcc.hash);
		}
		else if (strncmp(p, "#else", 5) == 0 && (p[5] == '\0' || p[5] <= ' '))
		{
			processElse();
		}
		else if (strncmp(p, "#endif", 6) == 0 && (p[6] == '\0' || p[6] <= ' '))
		{
			processEndif();
		}
		else if (shouldSkip())
		{
			/* Skip source in an inactive conditional branch. */
			free(out);
			free(buf);
			continue;
		}
		else if (strncmp(p, "#include", 8) == 0 && (p[8] == '\0' || p[8] <= ' '))
		{
			processInclude(srcfile, p + 8);
		}
		else if (strncmp(p, "#define", 7) == 0 && (p[7] == '\0' || p[7] <= ' '))
		{
			processDefine(p + 7, &mcc.hash);
		}
		else if (strncmp(p, "#undef", 6) == 0 && (p[6] == '\0' || p[6] <= ' '))
		{
			processUndef(p + 6, &mcc.hash);
		}
		else if (strncmp(p, "#pragma", 7) == 0 && (p[7] == '\0' || p[7] <= ' '))
		{
			processPragma(p + 7);
		}
		else if (strncmp(p, "#error", 6) == 0 && (p[6] == '\0' || p[6] <= ' '))
		{
			error("prepro", "%s", p + 6);
		}
		else if (strncmp(p, "#warning", 8) == 0 && (p[8] == '\0' || p[8] <= ' '))
		{
			if (cmd.warningsAsErrors)
			{
				error("prepro", "#warning:%s", p + 8);
			}
			fprintf(stderr, "warning: %s\n", p + 8);
		}
		else if (strncmp(p, "#line", 5) == 0 && (p[5] == '\0' || p[5] <= ' '))
		{
			/* #line NUMBER ["FILENAME"] */
			char *lp = p + 5;
			while (*lp && *lp <= ' ')
			{
				lp++;
			}
			int newLine = atoi(lp);
			if (newLine > 0)
			{
				nextLine = newLine;
			}
			while (*lp && *lp > ' ' && *lp != '"')
			{
				lp++;
			}
			while (*lp && *lp <= ' ')
			{
				lp++;
			}
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
					p = skipQuotedLiteral(p);
				}
				else if (!isIdentifierStart(*p))
				{
					if (*p++ == '(' && (strcmp(key, "main") == 0 || strcmp(key, "WinMain") == 0))
					{
						mcc.typeApp = *key == 'm' ? 3 : 2;
						mcc.mainfile = nFile;
					}
				}
				else
				{
					for (q = key, pBgn = p; isIdentifierContinue(*p);)
					{
						if ((size_t)(q - key) + 1U >= sizeof(key))
						{
							error("prepro",
							      "identifier exceeds %u characters",
							      (unsigned int)sizeof(key) - 1U);
						}
						*q++ = *p++;
					}
					*q = '\0';
					char *val = NULL;
					char tmpval[4096];
					int freeArgs = 0;
					char *args[MAX_MACRO_PARAMS];
					int nArgs = 0;

					/* ISO C99 predefined macros */
					if (strcmp(key, "__FILE__") == 0)
					{
						int written = snprintf(tmpval, sizeof(tmpval), "\"%s\"", srcfile);
						if (written < 0 || (size_t)written >= sizeof(tmpval))
						{
							error("prepro", "expanded __FILE__ value is too long");
						}
						val = tmpval;
					}
					else if (strcmp(key, "__LINE__") == 0)
					{
						int written = snprintf(tmpval, sizeof(tmpval), "%d", nLine);
						if (written < 0 || (size_t)written >= sizeof(tmpval))
						{
							error("prepro", "expanded __LINE__ value is too long");
						}
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
						/* Function-like macro */
						MacroDef *m = findMacro(key);
						char *invocation = p;
						while (*invocation == ' ' || *invocation == '\t')
						{
							++invocation;
						}
						if (m != NULL && *invocation == '(')
						{
							char *endArgs;
							nArgs = extractMacroArgs(invocation, args, MAX_MACRO_PARAMS, &endArgs);
							if (nArgs == 0 && m->parameterCount == 1)
							{
								args[nArgs++] = xstrdup("");
							}
							if ((!m->variadic && nArgs != m->parameterCount) ||
							    (m->variadic && nArgs < m->parameterCount))
							{
								error("prepro",
								      "macro '%s' expects %d argument%s, got %d",
								      m->name,
								      m->parameterCount,
								      m->parameterCount == 1 ? "" : "s",
								      nArgs);
							}
							freeArgs = 1;
							expandFunctionMacro(m, args, nArgs, tmpval, sizeof(tmpval));
							val = tmpval;
							p = endArgs;
						}
						else
						{
							val = hashGet(key, &mcc.hash);
						}
					}
					if (val != NULL)
					{
						size_t vlen = strlen(val);
						size_t prefix = (size_t)(pBgn - out);
						size_t suffix = strlen(p);
						if (++expansionCount > 4096)
						{
							error("prepro",
							      "recursive or excessive macro expansion involving '%s'",
							      key);
						}
						if (prefix + vlen + suffix + 1U > outCapacity)
						{
							size_t pOffset = (size_t)(p - out);
							size_t required = prefix + vlen + suffix + 1U;
							while (outCapacity < required)
							{
								if (outCapacity > SIZE_MAX / 2U)
								{
									error("prepro", "expanded source line is too large");
								}
								outCapacity *= 2U;
							}
							out = xrealloc(out, outCapacity);
							pBgn = out + prefix;
							p = out + pOffset;
						}
						memmove(pBgn + vlen, p, suffix + 1U);
						memmove(pBgn, val, vlen);
						p = strcmp(key, val) == 0 ? pBgn + vlen : pBgn;
					}
					/* Release parsed arguments. */
					if (freeArgs)
					{
						for (int i = 0; i < nArgs; i++)
						{
							free(args[i]);
						}
					}
				}
			}
			addLine(out, nFile, nLine);
		}
		free(out);
		free(buf);
	}
	if (fComment)
	{
		error("prepro", "unterminated block comment in '%s'", srcfile);
	}
	if (conditionalStack.depth != initialIfDepth)
	{
		error("prepro", "unterminated conditional directive in '%s'", srcfile);
	}
	fclose(fpSrc);
	if (opt & oDLL)
	{
		mcc.typeApp = 0;
	}
	if (opt & oLINES && strstr(srcfile, "include\\") == NULL)
	{
		mcc.totalLines += nextLine - 1;
		printf("%-24s\t%5d\n", srcfile, nextLine - 1);
	}
}

/* Initialization and target startup */

void initPrepro(void)
{
	mcc.sizeSrcLine = 1000;
	mcc.pSrcLine = xalloc(mcc.sizeSrcLine * sizeof(SRCLINE));
	hashPut("__CC__", "1", &mcc.hash);
	hashPut("__STDC__", "1", &mcc.hash);
	hashPut("__STDC_VERSION__", "199901L", &mcc.hash);
	hashPut("__STDC_HOSTED__", "1", &mcc.hash);
	cmd.target->definePredefinedMacros(&compiler);

	/* Initialize __DATE__ and __TIME__. */
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	static const char *months[] = {
	    "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	if (tm == NULL)
	{
		error("prepro", "cannot read the local time");
	}
	sprintf(macro_date, "\"%s %2d %d\"", months[tm->tm_mon], tm->tm_mday, tm->tm_year + 1900);
	sprintf(macro_time, "\"%02d:%02d:%02d\"", tm->tm_hour, tm->tm_min, tm->tm_sec);

	cmd.target->addRuntimePrelude(&compiler);
}

void addStartup(void)
{
	cmd.target->addStartup(&compiler);
}

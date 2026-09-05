/* Converts preprocessed source into the compiler token stream. */

#include "cc.h"

/* Debug output */

void printToken(int n)
{
	printf("%3d %2d:%03d %d \t%s\n",
	       n + 1,
	       cd.token[n].filenumber,
	       cd.token[n].linenumber,
	       cd.token[n].type,
	       cd.token[n].token);
	fflush(stdout);
}

/* Token classification */

void setToken(TOKEN *token, HASH *pHash)
{
	char *tkstr = token->token;
	if (token->type == TK_SYMBOL || token->type == TK_NAME)
	{
		token->ival = hashPut(tkstr, NULL, pHash);
	}
	else if (token->type == TK_NUMBER)
	{
		char *end;
		int hexadecimal = tkstr[0] == '0' && (tkstr[1] == 'x' || tkstr[1] == 'X');
		int floating = strchr(tkstr, '.') != NULL ||
		               (hexadecimal ? strchr(tkstr, 'p') != NULL || strchr(tkstr, 'P') != NULL
		                            : strchr(tkstr, 'e') != NULL || strchr(tkstr, 'E') != NULL);
		if (floating)
		{
			token->type = TK_DOUBLE;
			token->dval = strtod(tkstr, &end);
			if (*end == 'f' || *end == 'F')
			{
				token->type = TK_FLOAT;
				token->dval = (double)(float)token->dval;
			}
			if (*end == 'f' || *end == 'F' || *end == 'l' || *end == 'L')
			{
				++end;
			}
		}
		else
		{
			const char *digits = tkstr;
			unsigned int value = 0U;
			unsigned int base = 10U;
			int hasUnsignedSuffix = FALSE;
			int longSuffixCount = 0;
			if (digits[0] == '0' && (digits[1] == 'x' || digits[1] == 'X'))
			{
				base = 16U;
				digits += 2;
			}
			else if (digits[0] == '0')
			{
				base = 8U;
			}
			end = (char *)digits;
			while (*end != '\0')
			{
				unsigned int digit;
				if (*end >= '0' && *end <= '9')
				{
					digit = (unsigned int)(*end - '0');
				}
				else if (*end >= 'a' && *end <= 'f')
				{
					digit = (unsigned int)(*end - 'a') + 10U;
				}
				else if (*end >= 'A' && *end <= 'F')
				{
					digit = (unsigned int)(*end - 'A') + 10U;
				}
				else
				{
					break;
				}
				if (digit >= base)
				{
					break;
				}
				if (value > (UINT_MAX - digit) / base)
				{
					error("lexer", "integer literal is out of the 32-bit target range: %s", tkstr);
				}
				value = value * base + digit;
				++end;
			}
			if (end == digits)
			{
				error("lexer", "invalid numeric literal: %s", tkstr);
			}
			while (*end != '\0')
			{
				if (*end == 'u' || *end == 'U')
				{
					if (hasUnsignedSuffix)
					{
						error("lexer", "duplicate integer suffix: %s", tkstr);
					}
					hasUnsignedSuffix = TRUE;
					++end;
				}
				else if (*end == 'l' || *end == 'L')
				{
					++longSuffixCount;
					++end;
				}
				else
				{
					break;
				}
			}
			if (longSuffixCount > 1)
			{
				error(
				    "lexer", "64-bit integer literal is not supported by the 32-bit IR: %s", tkstr);
			}
			if (!hasUnsignedSuffix && value > (unsigned int)INT_MAX)
			{
				if (base == 10U)
				{
					error("lexer",
					      "decimal integer literal is out of the signed 32-bit target range: %s",
					      tkstr);
				}
				hasUnsignedSuffix = TRUE;
			}
			token->type = longSuffixCount != 0 ? (hasUnsignedSuffix ? TK_ULONG : TK_LONG)
			                                   : (hasUnsignedSuffix ? TK_UINT : TK_INT);
			token->ival = (int)value;
		}
		if (*end != '\0')
		{
			error("lexer", "invalid numeric literal: %s", tkstr);
		}
	}
	else if (tkstr[0] == '\'')
	{
		token->numericEscape = tkstr[1] == '\\' && ((tkstr[2] >= '0' && tkstr[2] <= '7') ||
		                                            tkstr[2] == 'x' || tkstr[2] == 'X');
		int value = tkstr[1] != '\\' ? (unsigned char)tkstr[1] : decodeEscape(tkstr + 2);
		token->type = TK_CHAR;
		token->ival = value;
	}
	else
	{
		char *pTail = &tkstr[strlen(tkstr) - 1];
		if (*pTail == '\"')
		{
			*pTail = '\0';
		}
		token->ival = hashPut(tkstr + 1, NULL, pHash);
	}
}

static char *scanNumber(char *p)
{
	char *q = p;
	int hexadecimal = q[0] == '0' && (q[1] == 'x' || q[1] == 'X');
	if (hexadecimal)
	{
		q += 2;
		while (isxdigit((unsigned char)*q))
		{
			++q;
		}
		if (*q == '.')
		{
			++q;
			while (isxdigit((unsigned char)*q))
			{
				++q;
			}
		}
		if (*q == 'p' || *q == 'P')
		{
			++q;
			if (*q == '+' || *q == '-')
			{
				++q;
			}
			while (isdigit((unsigned char)*q))
			{
				++q;
			}
		}
	}
	else
	{
		while (isdigit((unsigned char)*q))
		{
			++q;
		}
		if (*q == '.')
		{
			++q;
			while (isdigit((unsigned char)*q))
			{
				++q;
			}
		}
		if (*q == 'e' || *q == 'E')
		{
			++q;
			if (*q == '+' || *q == '-')
			{
				++q;
			}
			while (isdigit((unsigned char)*q))
			{
				++q;
			}
		}
	}
	while (*q == 'u' || *q == 'U' || *q == 'l' || *q == 'L' || *q == 'f' || *q == 'F')
	{
		++q;
	}
	if (isIdentifierStart((unsigned char)*q))
	{
		while (isIdentifierContinue((unsigned char)*q))
		{
			++q;
		}
	}
	return q;
}

static void appendToken(
    int type, char *begin, char *end, const SRCLINE *source, int *tokenCount, int *tokenCapacity)
{
	size_t tokenLength = (size_t)(end - begin);
	char *text = xalloc(tokenLength + 1U);
	memcpy(text, begin, tokenLength);
	text[tokenLength] = '\0';

	if (*tokenCount > 0 && cd.token[*tokenCount - 1].type == TK_STRING && type == TK_STRING)
	{
		char *previous = cd.token[*tokenCount - 1].token;
		size_t previousLength = strlen(previous);
		previous = xrealloc(previous, previousLength + tokenLength);
		strcpy(previous + previousLength - 1U, text + 1);
		cd.token[*tokenCount - 1].token = previous;
		free(text);
		return;
	}

	if (*tokenCount >= *tokenCapacity - 1)
	{
		*tokenCapacity = *tokenCapacity * 3 / 2;
		cd.token = xrealloc(cd.token, (size_t)*tokenCapacity * sizeof(TOKEN));
	}
	{
		TOKEN token = {type, 0, 0, 0.0, text, source->filenumber, source->linenumber};
		memcpy(&cd.token[*tokenCount], &token, sizeof(TOKEN));
		*tokenCount += 1;
	}
}

/* Lexer */

void lex(void)
{
	char *p, *pBgn, op2[4];
	int n, type, nToken = 0, sizeToken = 1000;

	cd.token = xalloc(sizeToken * sizeof(TOKEN));
	for (n = 0; n < mcc.nSrcLine; n++)
	{
		SRCLINE *pSrc = &mcc.pSrcLine[n];
		char *lineEnd = pSrc->srccode + strlen(pSrc->srccode);
		for (p = pSrc->srccode; *p != '\0';)
		{
			if (*p <= ' ')
			{
				p++;
				continue;
			}
			pBgn = p;
			if (*p == '\"' || *p == '\'')
			{
				type = TK_STRING;
				p = skipQuotedLiteral(pBgn);
			}
			else if (isdigit((unsigned char)*p) || (*p == '.' && isdigit((unsigned char)p[1])))
			{
				type = TK_NUMBER;
				p = scanNumber(p);
			}
			else if (isIdentifierStart(*p))
			{
				type = TK_NAME;
				for (p++; *p != '\0' && isIdentifierContinue(*p);)
				{
					p++;
				}
			}
			else if (strncmp(p, "...", 3) == 0)
			{
				type = TK_NAME;
				p += 3;
			}
			else if (strncmp(p, "<<=", 3) == 0 || strncmp(p, ">>=", 3) == 0)
			{
				type = TK_SYMBOL;
				p += 3;
			}
			else
			{
				type = TK_SYMBOL;
				op2[0] = p[0];
				op2[1] = p[1];
				op2[2] = ' ';
				op2[3] = '\0';
				p += p[1] != '\0' && strstr(OPERATOR2, op2) != NULL ? 2 : 1;
			}
			if (p <= pBgn || p > lineEnd)
			{
				error("lexer",
				      "token scanner crossed the end of %s:%d near '%s'",
				      pSrc->filenumber >= 0 ? mcc.srcFile[pSrc->filenumber] : "startup",
				      pSrc->linenumber,
				      pBgn);
			}
			appendToken(type, pBgn, p, pSrc, &nToken, &sizeToken);
		}
	}
	cd.nToken = nToken;
}

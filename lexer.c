/*
 * lexer.c - Analisador Léxico do compilador
 * 
 * Responsável por tokenizar o código fonte preprocessado.
 */

#include "cc.h"

/*============================================================================
 * Funções de Debug
 *============================================================================*/

void printToken(int n)
{
	printf("%3d %2d:%03d %d \t%s\n", n + 1, cd.token[n].filenumber,
		cd.token[n].linenumber, cd.token[n].type, cd.token[n].token);
}

/*============================================================================
 * Configuração de Token
 *============================================================================*/

void setToken(TOKEN *token, HASH *pHash)
{
	char *tkstr = token->token;
	if (token->type == TK_SYMBOL || token->type == TK_NAME)
	{
		token->ival = put(tkstr, NULL, pHash);
	}
	else if (token->type == TK_NUMBER)
	{
		token->type = strchr(tkstr, '.') != NULL ? TK_DOUBLE : TK_INT;
		if (token->type == TK_DOUBLE) token->dval = strtod(tkstr, NULL);
		else token->ival = strtoul(tkstr, NULL, tolower(tkstr[1]) == 'x' ? 16 : 10);
	}
	else if (tkstr[0] == '\'')
	{
		token->type = TK_CHAR;
		token->ival = tkstr[1] != '\\' ? tkstr[1] : esc_char(tkstr + 2);
	}
	else
	{
		char *pTail = &tkstr[strlen(tkstr) - 1];
		if (*pTail == '\"') *pTail = '\0';
		token->ival = put(tkstr + 1, NULL, pHash);
	}
}

/*============================================================================
 * Analisador Léxico Principal
 *============================================================================*/

void lex(void)
{
	char *p, *pBgn, op2[4], token[256];
	int n, type, nToken = 0, sizeToken = 1000;

	cd.token = xalloc(sizeToken * sizeof(TOKEN));
	for (n = 0; n < mcc.nSrcLine; n++)
	{
		SRCLINE *pSrc = &mcc.pSrcLine[n];
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
				p = endOfQuote(pBgn);
			}
			else if (isdigit(*p))
			{
				type = TK_NUMBER;
				if (*p == '0' && (p[1] == 'x' || p[1] == 'X')) strtol(p, &p, 16);
				else strtod(p, &p);
			}
			else if (isAlpha(*p))
			{
				type = TK_NAME;
				for (p++; *p != '\0' && isAlNum(*p);) p++;
			}
			else if (strncmp(p, "...", 3) == 0)
			{
				type = TK_NAME;
				p += 3;
			}
			else
			{
				type = TK_SYMBOL;
				sprintf(op2, "%c%c ", p[0], p[1]);
				p += strstr(OPERATOR2, op2) != NULL ? 2 : 1;
			}
			if (nToken >= sizeToken - 1)
			{
				sizeToken = sizeToken * 3 / 2;
				cd.token = xrealloc(cd.token, sizeToken * sizeof(TOKEN));
			}
			sprintf(token, "%.*s", (int)(p - pBgn), pBgn);
			if (nToken > 0 && cd.token[nToken - 1].type == TK_STRING && type == TK_STRING)
			{
				char *tk1 = cd.token[nToken - 1].token;
				int len = strlen(tk1);
				char *tk = xrealloc(tk1, len + (p - pBgn));
				strcpy(tk + len - 1, token + 1);
				cd.token[nToken - 1].token = tk;
			}
			else
			{
				TOKEN tkn = { type, 0, 0.0, xstrdup(token), pSrc->filenumber, pSrc->linenumber };
				memcpy(&cd.token[nToken++], &tkn, sizeof(TOKEN));
			}
		}
	}
	cd.nToken = nToken;
}

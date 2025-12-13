/*
 * util.c - Funções utilitárias do compilador
 * 
 * Contém funções de alocação de memória, manipulação de strings,
 * hash tables e tratamento de erros.
 */

#include "cc.h"

/*============================================================================
 * Tratamento de Erros
 *============================================================================*/

void error(const char *loc, const char *format, ...)
{
	if ((opt&oDEBUG) && loc != NULL) fprintf(stderr, "%s: ", loc);
	if (0 <= ix.tix && ix.tix < cd.nToken)
	{
		int filenumber = cd.token[ix.tix].filenumber;
		char *srcfile = filenumber >= 0 ? mcc.srcFile[filenumber] : "startup";
		fprintf(stderr, "%s:%d: ", srcfile, cd.token[ix.tix].linenumber);
	}
	else if (mcc.nPreFile >= 0)
	{
		fprintf(stderr, "%s:%d: ", mcc.srcFile[mcc.nPreFile], mcc.lines[mcc.nPreFile]);
	}
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	exit(1);
}

/*============================================================================
 * Funções de Caracteres
 *============================================================================*/

int htoi(int c)
{
	return c >= 'A' ? (c - 'A' + 10) : (c - '0');
}

int esc_char(char *p)
{
	if (*p == 'x' || *p == 'X') return (htoi(p[1]) << 4) + htoi(p[2]);
	return *p == '0' ? '\0' : *p == 'r' ? '\r' : *p == 'n' ? '\n' : *p == 't' ? '\t' : *p;
}

int isAlpha(int c)
{
	return (isalpha(c) || c == '_');
}

int isAlNum(int c)
{
	return isAlpha(c) || isdigit(c);
}

int isKanji(int c)
{
	return (c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC);
}

char *endOfQuote(char *p)
{
	int delim = *p++;
	while (*p != '\0' && *p != delim)
		p += (*p == '\\' || isKanji(*p & 0xff)) ? 2 : 1;
	if (*p != delim) error("endOfQuote", "missing terminating %c character", delim);
	return ++p;
}

/*============================================================================
 * Funções de Alocação de Memória
 *============================================================================*/

void *xcheck(void *p)
{
	if (p == NULL) error("xcheck", "");
	return p;
}

void *xalloc(int size)
{
	return xcheck(calloc(1, size));
}

void *xrealloc(void *ptr, size_t size)
{
	return xcheck(realloc(ptr, size));
}

char *xstrdup(char *q)
{
	return xcheck(strdup(q));
}

/*============================================================================
 * Funções de String
 *============================================================================*/

int xstrcpy(char *p, char *q)
{
	int len = 0;
	while (*q)
	{
		int fKanji = isKanji(*q & 0xff);
		len += fKanji ? 2 : 1;
		if (p != NULL && fKanji)
		{
			*p++ = *q;
			*p++ = q[1];
		}
		else if (p != NULL)
		{
			*p++ = *q == '\\' ? esc_char(q + 1) : *q;
		}
		q += *q == '\\' ? (q[1] == 'x' ? 4 : 2) : (fKanji ? 2 : 1);
	}
	return len;
}

int xstrlen(char *p)
{
	return xstrcpy(NULL, p);
}

/*============================================================================
 * Hash Table
 *============================================================================*/

void initHash(int type, int size, HASH *pHash)
{
	pHash->type = type;
	pHash->size = size;
	pHash->tbl = xalloc(size * sizeof(HDATA));
	pHash->entries = 0;
}

void reallocHash(int newSize, HASH *pHash)
{
	int  n, oldSize = pHash->size;
	HDATA *oldTbl = pHash->tbl;
	initHash(pHash->type, newSize, pHash);
	for (n = 0; n < oldSize; n++)
	{
		if (oldTbl[n].key != NULL) put(oldTbl[n].key, oldTbl[n].val, pHash);
	}
	free(oldTbl);
}

void freeHash(HASH *pHash)
{
	int n;
	for (n = 0; n < pHash->size; n++)
	{
		if (pHash->tbl[n].key != NULL) free(pHash->tbl[n].key);
		if ((pHash->type == 's' || pHash->type == 'n') && pHash->tbl[n].val != 0)
			free(pHash->tbl[n].val);
	}
	free(pHash->tbl);
}

int hash(char *key, int size)
{
	int n, h = 0;
	for (n = 0; key[n] != '\0'; n++)
		h = (h * 137 + (key[n] & 0xff)) % size;
	return h;
}

int put(char *key, void *val, HASH *pHash)
{
	int n, h = hash(key, pHash->size);
	for (n = 0; n < pHash->size; n++)
	{
		int ix = (h + n) % pHash->size;
		if (pHash->tbl[ix].key == NULL)
		{
			pHash->tbl[ix].key = xstrdup(key);
			pHash->tbl[ix].val = pHash->type == 's' ? xstrdup(val) : val;
			pHash->tbl[ix].seq = pHash->entries++;
			if (pHash->type != 'x' && pHash->entries > pHash->size / 2)
				reallocHash(pHash->size * 2, pHash);
			return ix;
		}
		else if (strcmp(pHash->tbl[ix].key, key) == 0)
		{
			return ix;
		}
	}
	error("lib.put", "Hash Table full!");
	return -1;
}

void *get(char *key, HASH *pHash)
{
	int n, h = hash(key, pHash->size);
	for (n = 0; n < pHash->size; n++)
	{
		int ix = (h + n) % pHash->size;
		if (pHash->tbl[ix].key == NULL) break;
		if (strcmp(pHash->tbl[ix].key, key) == 0)
			return pHash->tbl[ix].val;
	}
	return NULL;
}

int del(char *key, HASH *pHash)
{
	int n, h = hash(key, pHash->size);
	for (n = 0; n < pHash->size; n++)
	{
		int ix = (h + n) % pHash->size;
		if (pHash->tbl[ix].key == NULL) break;
		if (strcmp(pHash->tbl[ix].key, key) == 0)
		{
			free(pHash->tbl[ix].key);
			pHash->tbl[ix].key = NULL;
			if (pHash->type == 's' && pHash->tbl[ix].val != NULL)
				free(pHash->tbl[ix].val);
			pHash->tbl[ix].val = NULL;
			pHash->entries--;
			return 1;
		}
	}
	return 0;
}

void printHash(HASH *pHash)
{
	char *fmt = pHash->type == 's' ? "%4d %-8s %s\n" : "%4d %-8s %08X\n";
	int n;
	for (n = 0; n < pHash->size; n++)
	{
		if (pHash->tbl[n].key != NULL)
			printf(fmt, n, pHash->tbl[n].key, pHash->tbl[n].val);
	}
}

/* Shared allocation, text, hash-table, and diagnostic helpers. */

#include "cc.h"

/* Diagnostics */

CC_NORETURN void error(const char *loc, const char *format, ...)
{
	if ((opt & oDEBUG) && loc != NULL)
	{
		fprintf(stderr, "%s: ", loc);
	}
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

/* Character handling */

int hexDigitValue(int c)
{
	if (c >= '0' && c <= '9')
	{
		return c - '0';
	}
	if (c >= 'a' && c <= 'f')
	{
		return c - 'a' + 10;
	}
	if (c >= 'A' && c <= 'F')
	{
		return c - 'A' + 10;
	}
	error("hexDigitValue", "invalid hexadecimal digit 0x%02X", (unsigned int)(unsigned char)c);
}

int decodeEscape(char *p)
{
	const char *cursor = p;
	return decodeEscapeSequence(&cursor);
}

int decodeEscapeSequence(const char **cursor)
{
	const char *text = *cursor;
	int value;
	if (*text == 'x' || *text == 'X')
	{
		++text;
		if (!isxdigit((unsigned char)*text))
		{
			error("escape", "hexadecimal escape requires at least one digit");
		}
		value = 0;
		while (isxdigit((unsigned char)*text))
		{
			value = value * 16 + hexDigitValue(*text++);
		}
		*cursor = text;
		return value;
	}
	if (*text >= '0' && *text <= '7')
	{
		int digits = 0;
		value = 0;
		while (digits < 3 && *text >= '0' && *text <= '7')
		{
			value = value * 8 + (*text++ - '0');
			++digits;
		}
		*cursor = text;
		return value;
	}
	value = *text == 'a'   ? '\a'
	        : *text == 'b' ? '\b'
	        : *text == 'f' ? '\f'
	        : *text == 'n' ? '\n'
	        : *text == 'r' ? '\r'
	        : *text == 't' ? '\t'
	        : *text == 'v' ? '\v'
	                       : (unsigned char)*text;
	if (*text != '\0')
	{
		++text;
	}
	*cursor = text;
	return value;
}

int isIdentifierStart(int c)
{
	return isalpha((unsigned char)c) || c == '_';
}

int isIdentifierContinue(int c)
{
	return isIdentifierStart(c) || isdigit((unsigned char)c);
}

int isShiftJisLeadByte(int c)
{
	return (c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC);
}

char *skipQuotedLiteral(char *p)
{
	int delim = *p++;
	while (*p != '\0' && *p != delim)
	{
		p += (*p == '\\' || isShiftJisLeadByte(*p & 0xff)) ? 2 : 1;
	}
	if (*p != delim)
	{
		error("skipQuotedLiteral", "missing terminating %c character", delim);
	}
	return ++p;
}

/* Memory allocation */

static void *xcheck(void *p)
{
	if (p == NULL)
	{
		error("xcheck", "");
	}
	return p;
}

void *xalloc(size_t size)
{
	if (size == 0U)
	{
		size = 1U;
	}
	return xcheck(calloc(1, size));
}

void *xrealloc(void *ptr, size_t size)
{
	return xcheck(realloc(ptr, size));
}

char *xstrdup(const char *q)
{
	size_t size;
	char *copy;
	if (q == NULL)
	{
		error("xstrdup", "null string");
	}
	size = strlen(q) + 1U;
	copy = xalloc(size);
	memcpy(copy, q, size);
	return copy;
}

/* String-literal decoding */

int decodeString(char *p, char *q)
{
	int len = 0;
	while (*q)
	{
		int fKanji = isShiftJisLeadByte(*q & 0xff);
		int value = 0;
		const char *next = q;
		if (*q == '\\')
		{
			++next;
			value = decodeEscapeSequence(&next);
		}
		len += fKanji ? 2 : 1;
		if (p != NULL && fKanji)
		{
			*p++ = *q;
			*p++ = q[1];
		}
		else if (p != NULL)
		{
			*p++ = (char)(*q == '\\' ? value : (unsigned char)*q);
		}
		q = *q == '\\' ? (char *)next : q + (fKanji ? 2 : 1);
	}
	return len;
}

int decodedStringLength(char *p)
{
	return decodeString(NULL, p);
}

/* Hash table */

void hashInit(int type, int size, HASH *pHash)
{
	if (pHash == NULL || size < 4)
	{
		error("hashInit", "invalid hash-table size");
	}
	pHash->type = type;
	pHash->size = size;
	pHash->tbl = xalloc(size * sizeof(HDATA));
	memset(pHash->tbl, 0, (size_t)size * sizeof(HDATA));
	pHash->entries = 0;
	pHash->nextSequence = 0;
}

static void resizeHash(int newSize, HASH *pHash)
{
	int n, oldSize = pHash->size;
	int oldEntries = pHash->entries;
	int oldNextSequence = pHash->nextSequence;
	int type = pHash->type;
	HDATA *oldTbl = pHash->tbl;
	hashInit(type, newSize, pHash);
	for (n = 0; n < oldSize; n++)
	{
		if (oldTbl[n].state == 1U)
		{
			int slot = hashPut(oldTbl[n].key, oldTbl[n].val, pHash);
			pHash->tbl[slot].seq = oldTbl[n].seq;
			free(oldTbl[n].key);
			if (type == 's')
			{
				free(oldTbl[n].val);
			}
		}
	}
	free(oldTbl);
	if (pHash->entries != oldEntries)
	{
		error("resizeHash", "hash-table entry count changed while rehashing");
	}
	pHash->nextSequence = oldNextSequence;
}

void hashFree(HASH *pHash)
{
	int n;
	for (n = 0; n < pHash->size; n++)
	{
#if defined(_MSC_VER)
#pragma warning(suppress : 6001) /* hashInit zero-initializes every slot. */
#endif
		if (pHash->tbl[n].state == 1U)
		{
			free(pHash->tbl[n].key);
		}
#if defined(_MSC_VER)
#pragma warning(suppress : 6001) /* hashInit zero-initializes every slot. */
#endif
		if (pHash->tbl[n].state == 1U && (pHash->type == 's' || pHash->type == 'n') &&
		    pHash->tbl[n].val != NULL)
		{
			free(pHash->tbl[n].val);
		}
	}
	free(pHash->tbl);
}

static int hashIndex(char *key, int size)
{
	int n, h = 0;
	for (n = 0; key[n] != '\0'; n++)
	{
		h = (h * 137 + (key[n] & 0xff)) % size;
	}
	return h;
}

int hashPut(char *key, void *val, HASH *pHash)
{
	int n, h;
	int firstDeleted = -1;
	if (key == NULL || pHash == NULL || pHash->tbl == NULL)
	{
		error("hashPut", "invalid hash-table insertion");
	}
	h = hashIndex(key, pHash->size);
	for (n = 0; n < pHash->size; n++)
	{
		int slot = (h + n) % pHash->size;
		if (pHash->tbl[slot].state == 2U)
		{
			if (firstDeleted < 0)
			{
				firstDeleted = slot;
			}
			continue;
		}
		if (pHash->tbl[slot].state == 0U)
		{
			if (firstDeleted >= 0)
			{
				slot = firstDeleted;
			}
			pHash->tbl[slot].key = xstrdup(key);
			pHash->tbl[slot].val =
			    pHash->type == 's' ? xstrdup(val != NULL ? (char *)val : "") : val;
			pHash->tbl[slot].seq = pHash->nextSequence++;
			pHash->entries++;
			pHash->tbl[slot].state = 1U;
			if (pHash->type != 'x' && pHash->entries > pHash->size / 2)
			{
				if (pHash->size > INT_MAX / 2)
				{
					error("hashPut", "hash table is too large");
				}
				resizeHash(pHash->size * 2, pHash);
				return hashPut(key, val, pHash);
			}
			return slot;
		}
		else if (strcmp(pHash->tbl[slot].key, key) == 0)
		{
			if (pHash->type == 's')
			{
				char *replacement = xstrdup(val != NULL ? (char *)val : "");
				free(pHash->tbl[slot].val);
				pHash->tbl[slot].val = replacement;
			}
			return slot;
		}
	}
	if (firstDeleted >= 0)
	{
		pHash->tbl[firstDeleted].key = xstrdup(key);
		pHash->tbl[firstDeleted].val =
		    pHash->type == 's' ? xstrdup(val != NULL ? (char *)val : "") : val;
		pHash->tbl[firstDeleted].seq = pHash->nextSequence++;
		pHash->entries++;
		pHash->tbl[firstDeleted].state = 1U;
		return firstDeleted;
	}
	error("hashPut", "hash table is full");
}

void *hashGet(char *key, HASH *pHash)
{
	int n, h = hashIndex(key, pHash->size);
	for (n = 0; n < pHash->size; n++)
	{
		int slot = (h + n) % pHash->size;
		if (pHash->tbl[slot].state == 0U)
		{
			break;
		}
		if (pHash->tbl[slot].state == 1U && strcmp(pHash->tbl[slot].key, key) == 0)
		{
			return pHash->tbl[slot].val;
		}
	}
	return NULL;
}

int hashRemove(char *key, HASH *pHash)
{
	int n, h = hashIndex(key, pHash->size);
	for (n = 0; n < pHash->size; n++)
	{
		int slot = (h + n) % pHash->size;
		if (pHash->tbl[slot].state == 0U)
		{
			break;
		}
		if (pHash->tbl[slot].state == 1U && strcmp(pHash->tbl[slot].key, key) == 0)
		{
			free(pHash->tbl[slot].key);
			pHash->tbl[slot].key = NULL;
			if (pHash->type == 's' && pHash->tbl[slot].val != NULL)
			{
				free(pHash->tbl[slot].val);
			}
			pHash->tbl[slot].val = NULL;
			pHash->tbl[slot].state = 2U;
			pHash->entries--;
			return 1;
		}
	}
	return 0;
}

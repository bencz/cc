/*
 * parser.c - Parser do compilador
 * 
 * Responsável pela análise sintática, declarações de variáveis,
 * funções, estruturas e statements.
 */

#include "cc.h"

/*============================================================================
 * Funções Auxiliares de Token
 *============================================================================*/

char *toStr(int tix)
{
	return cd.token[tix].token;
}

char *toString(int id)
{
	return cd.hash.tbl[id].key;
}

int id2(char* op)
{
	return (op[0] * 137 + op[1]) % cd.hash.size;
}

int isId(int n)
{
	return cd.token[ix.tix + n].type <= TK_NAME;
}

int is(int id)
{
	return isId(0) && cd.token[ix.tix].ival == id;
}

int isN(int id, int n)
{
	return isId(n) && cd.token[ix.tix + n].ival == id;
}

int is2(char *op)
{
	return isId(0) && cd.token[ix.tix].ival == id2(op);
}

int id(char* str)
{
	return put(str, NULL, &cd.hash);
}

int ispp(int id)
{
	if (is(id))
	{
		ix.tix++;
		return 1;
	}
	else return 0;
}

int is2pp(char *op)
{
	if (is2(op))
	{
		ix.tix++;
		return 1;
	}
	else return 0;
}

void skip(int id)
{
	if (!ispp(id)) error("code.skip", "'%s' expected", toString(id));
}

/*============================================================================
 * Funções de Tipo
 *============================================================================*/

int sizeOfDataType(int type)
{
	if (type == ID.CHAR || type == ID.HVOID) return 1;
	if (type == ID.SHORT)  return 2;
	if (type == ID.INT || type == ID.FLOAT) return 4;
	if (type == ID.DOUBLE) return 8;
	Name *pName = getStruct(type);
	return pName != NULL ? pName->size[0] : 0;
}

int isTypeSpecifier(int ix)
{
	int hix = cd.token[ix].ival;
	return cd.token[ix].type == TK_NAME && ((intptr_t)cd.hash.tbl[hix].val)&AT_TYPE;
}

/*============================================================================
 * Tabela de Nomes
 *============================================================================*/

static int cmpSeq(const void *a, const void *b)
{
	return ((HDATA*)a)->seq - ((HDATA*)b)->seq;
}

void printNameTable(int ixBlk)
{
	Block blk = cd.block[ixBlk];
	printf("------ %-16s#%d [%2d]-------\n", toString(blk.idFunc), blk.blockDepth, ixBlk);
	printf("%25s  %-25s %s\n", "dtype", "name", "ptrs atype   addr    size");
	qsort(blk.hash.tbl, blk.hash.size, sizeof(HDATA), cmpSeq);
	int n;
	for (n = 0; n < blk.hash.size; n++)
	{
		Name *e = blk.hash.tbl[n].val;
		if (e == NULL) continue;
		char *fmt = strlen(toString(e->dataType)) < 26 ? "%25s  %-25s" : "%.23s..  %.23s..";
		printf(fmt, toString(e->dataType), toString(e->idName));
		printf("   %d%5d%9d%8d\n", e->ptrs, e->addrType, e->address, e->size[0]);
	}
}

void createNameTable(int idFunc)
{
	if (++cd.currTable > 1) idFunc = cd.block[cd.currTable - 1].idFunc;
	cd.block[cd.currTable].idFunc = idFunc;
	cd.block[cd.currTable].blockDepth = cd.currTable;
	initHash('n', cd.currTable == 0 ? 1000 : 4, &cd.block[cd.currTable].hash);
}

void deleteNameTable(void)
{
	if (opt&oNAME) printNameTable(cd.currTable);
	freeHash(&cd.block[cd.currTable--].hash);
}

Name *newName(int type, int dataType, int name, int addrType, int address)
{
	Name buf = { type, name, dataType, addrType, address };
	Name *pNew = xalloc(sizeof(Name));
	memcpy(pNew, &buf, sizeof(Name));
	pNew->argc = -1;
	return pNew;
}

Name *putName(int ixBlk, Name *pName)
{
	char key[128];
	int type = pName->type;
	char *name = toString((type&NM_STRUCT) ? pName->dataType : pName->idName);
	sprintf(key, "%s%s", (type&NM_FUNC) ? "@" : (type&NM_STRUCT) ? "$" : "", name);
	put(key, pName, &cd.block[ixBlk].hash);
	return pName;
}

Name *appendName(int ixBlk, int type, int dataType, int name, int addrType, int address)
{
	return putName(ixBlk, newName(type, dataType, name, addrType, address));
}

Name *getNameFromTable(int ixBlk, int type, int name)
{
	char key[128];
	sprintf(key, "%s%s", (type&NM_FUNC) ? "@" : (type&NM_STRUCT) ? "$" : "", toString(name));
	return get(key, &cd.block[ixBlk].hash);
}

Name *getNameFromAllTable(int ixBlk, int type, int name)
{
	for (; ixBlk >= 0; ixBlk--)
	{
		Name *e = getNameFromTable(ixBlk, type, name);
		if (e != NULL) return e;
	}
	return NULL;
}

Name *getStruct(int type)
{
	return getNameFromTable(globTable, NM_STRUCT, type);
}

int getPtr(int type)
{
	Name *pStruct = getStruct(type);
	return pStruct != NULL ? pStruct->ptrs : 0;
}

Name *getAttr(int type, int name)
{
	Name *e = getStruct(type)->pBgn;
	while (e != NULL && e->idName != name) e = e->pNext;
	return e;
}

Name *getMember(int type, int n)
{
	Name *e = getStruct(type)->pBgn;
	while (n-- > 0 && e != NULL) e = e->pNext;
	return e;
}

/*============================================================================
 * Expressões Constantes
 *============================================================================*/

void constExpression(VALUE *pv)
{
	int ixCodeSave = ix.ixCode;
	int ixDataSave = ix.ixData;
	conditionalExpression(VAL, pv);
	ix.ixCode = ixCodeSave;
	ix.ixData = ixDataSave;
}

int constIntExpression(void)
{
	VALUE v;
	constExpression(&v);
	if (!v.fConst || v.type != ID.INT)
		error("parser", "constant expression expected");
	return v.ival;
}

/*============================================================================
 * Alinhamento
 *============================================================================*/

int align(int pos, int width)
{
	if (pos == 0) return 0;
	if ((width & 7) == 0) return (pos + 7) & ~7;
	if ((width & 3) == 0) return (pos + 3) & ~3;
	if ((width & 1) == 0) return (pos + 1) & ~1;
	return pos;
}

/*============================================================================
 * Type Specifier
 *============================================================================*/

void typeSpecifier(void)
{
	if (is(ID.DECLSP)) ix.tix += 4;
	int fStruct = is(ID.STRUCT);
	if (fStruct || is(ID.TYPEDEF) || is(ID.HCONST)) ix.tix++;
	if (!fStruct && !isTypeSpecifier(ix.tix))
		error("typeSpecifier", "'%s' undeclared", toStr(ix.tix));
	var.type = cd.token[ix.tix++].ival;
}

/*============================================================================
 * Variable Declarator
 *============================================================================*/

static void setAttr(Name *pName)
{
	int n = var.size[0] == 0 ? 0 : var.arrays;
	pName->ptrs = var.pointers + var.arrays;
	pName->size[n] = var.width;
	while (--n >= 0) pName->size[n] = pName->size[n + 1] * var.size[n];
}

static void countMember(int depth, int *dim)
{
	VALUE v;
	int n;
	for (n = 0; !is('}'); n++)
	{
		if (ispp('{')) countMember(depth + 1, dim);
		else assignExpression(VAL, &v);
		if (!ispp(',')) break;
	}
	skip('}');
	if (++n > dim[depth]) dim[depth] = n;
}

int varDeclarator(int fParam)
{
	int n, fCount = FALSE, callConv = NM_CDECL;

	for (var.pointers = 0; ix.tix < cd.nToken && ispp('*');)
		var.pointers++;
	if (ispp(ID.HWINAPI)) callConv = NM_WINAPI;
	if (cd.token[ix.tix].type == TK_SYMBOL) var.id = -1;
	else var.id = cd.token[ix.tix++].ival;
	memset(&var.size, 0, sizeof(var.size));
	for (var.arrays = 0; var.arrays < 8 && ix.tix < cd.nToken && ispp('['); var.arrays++)
	{
		if (is(']'))
		{
			if (!fParam) fCount = TRUE;
		}
		else
		{
			var.size[var.arrays] = constIntExpression();
		}
		skip(']');
	}
	if (is('=') && fCount)
	{
		INDEX ixSave = ix;
		memset(var.size, 0, sizeof(var.size));
		if (cd.token[ix.tix + 1].type == TK_STRING)
		{
			var.size[0] = xstrlen(toString(cd.token[ix.tix + 1].ival)) + 1;
		}
		else
		{
			ix.tix += 2;
			countMember(0, var.size);
		}
		ix = ixSave;
	}
	if (fParam)
	{
		var.pointers += var.arrays;
		var.arrays = 0;
	}
	var.width = var.pointers > 0 ? 4 : sizeOfDataType(var.type);
	var.length = 1;
	for (n = 0; n < var.arrays; n++)
		var.length *= var.size[n];
	return callConv;
}

/*============================================================================
 * Initializer
 *============================================================================*/

static void initMember(Name *pName, int atype, int depth, int addr)
{
	VALUE v1, v2;
	setValue(VAL, pName->ptrs, pName->dataType, &v1);
	if (atype == AD_STACK)
	{
		int left_bgn = ix.ixCode;
		loadAddr(AD_STACK, addr);
		int left_end = ix.ixCode;
		outCode1(push_eax);
		assignExpression(VAL, &v2);
		infixOperation('=', &v1, &v2, left_bgn, left_end);
	}
	else
	{
		if (v1.ptrs - depth == 1 && v1.type == ID.CHAR)
		{
			if (cd.token[ix.tix].type == TK_STRING)
			{
				int ival = cd.token[ix.tix++].ival;
				outDataAddr(ix.ixString);
				outString(ix.ixString, toString(ival));
				ix.ixString += xstrlen(toString(ival)) + 1;
			}
			else
			{
				constExpression(&v2);
				outDataInt(v2.ival);
			}
		}
		else
		{
			constExpression(&v2);
			if (!v2.fConst) error("initMember", "constant expression expected");
			else if (v1.type == ID.DOUBLE)
				outDataDouble(v2.type == ID.DOUBLE ? v2.rval : (double)v2.ival);
			else if (v2.type != ID.INT) error("initMember", "type mismatch");
			else if (v1.type == ID.INT)   outDataInt(v2.ival);
			else if (v1.type == ID.SHORT) outDataShort(v2.ival);
			else if (v1.type == ID.CHAR)  outDataChar(v2.ival);
			else error("initMember", "v1.type=%d", v1.type);
		}
	}
}

static void clear(int bgn, int end)
{
	outCode2(push, end - bgn);
	outCode2(push, 0);
	outCode2(lea_eax_pbp, bgn);
	outCode1(push_eax);
	outCode2(call, id("memset"));
	outCode2(add_esp, 12);
}

void initializer(Name *pName, int atype, int depth, int addr)
{
	int  n, bgn;
	int  fArray = (pName->size[depth + 1] > 0);

	if (!ispp('{'))
	{
		if (fArray && pName->dataType == ID.CHAR && cd.token[ix.tix].type == TK_STRING)
		{
			int ival = cd.token[ix.tix++].ival;
			if (atype == AD_DATA)
			{
				ix.ixData += outString(ix.ixData, toString(ival)) + 1;
			}
			else
			{
				int size = pName->size[depth] / pName->size[depth + 1];
				int length = xstrlen(toString(ival)) + 1;
				if (length > size) error("init'r", "initializer-string too long");
				char *buf = xalloc(length);
				xstrcpy(buf, toString(ival));
				for (n = 0; n < size; n++)
				{
					outCode2(mov_eax, n < length ? buf[n] & 0xff : 0);
					outCode2(mov_pbp_al, addr + n);
				}
				free(buf);
			}
			return;
		}
		initMember(pName, atype, depth, addr);
		return;
	}
	if (fArray)
	{
		int size1 = pName->size[depth + 1];
		int num = pName->size[depth] / size1;
		for (n = 0; !is('}');)
		{
			if (n >= num) error("init'r", "index too large");
			initializer(pName, atype, depth + 1, addr + size1*n++);
			if (!ispp(',')) break;
		}
		bgn = size1 * n;
	}
	else
	{
		bgn = pName->size[depth];
		for (n = 0; !is('}');)
		{
			Name *pChild = getMember(pName->dataType, n++);
			if (pChild == NULL) error("init'r", "too many field init");
			initializer(pChild, atype, 0, addr + pChild->address);
			bgn = pChild->address + pChild->size[0];
			if (!ispp(',')) break;
		}
	}
	if (atype == AD_STACK && bgn < pName->size[depth])
		clear(addr + bgn, addr + pName->size[depth]);
	skip('}');
}

/*============================================================================
 * Variable Declaration
 *============================================================================*/

void variableDeclaration(Name *pStruct, int status)
{
	Name *pName;
	int fStatic = ispp(ID.STATIC);
	int fImp = is(ID.DECLSP);

	typeSpecifier();
	do
	{
		varDeclarator(FALSE);
		if (status == ST_FUNC)
		{
			if (!fStatic)
			{
				cd.baseSpace = -align(-cd.baseSpace, var.width);
				cd.baseSpace -= var.width * var.length;
				pName = newName(NM_VAR, var.type, var.id, AD_STACK, cd.baseSpace);
			}
			else
			{
				ix.ixData = align(ix.ixData, var.width);
				pName = newName(NM_VAR, var.type, var.id, AD_DATA, ix.ixData);
			}
			putName(cd.currTable, pName);
		}
		else if (fImp && status == ST_GVAR)
		{
			pName = appendName(globTable, NM_VAR, var.type, var.id, AD_IMPORT, 0);
		}
		else if (status == ST_GVAR)
		{
			ix.ixData = align(ix.ixData, var.width);
			pName = appendName(globTable, NM_VAR, var.type, var.id, AD_DATA, ix.ixData);
		}
		else
		{
			cd.offset = align(cd.offset, var.width);
			pName = xalloc(sizeof(Name));
			Name buf = { NM_ATTR, var.id, var.type, 0, cd.offset };
			memcpy(pName, &buf, sizeof(Name));
			if (pStruct->pBgn == NULL) pStruct->pBgn = pName;
			else pStruct->pEnd->pNext = pName;
			pStruct->pEnd = pName;
			cd.offset += var.width * var.length;
		}
		setAttr(pName);
		if (ispp('='))
		{
			int ixString = ix.ixData + pName->size[0];
			if (status == ST_GVAR) ix.ixString = ixString;
			initializer(pName, pName->addrType, 0, pName->address);
			if (ix.ixString > ixString) ix.ixData = ix.ixString;
		}
		else if (!fImp && (status == ST_GVAR || fStatic))
		{
			ix.ixZero = align(ix.ixZero, var.width);
			pName->addrType = AD_ZERO;
			pName->address = ix.ixZero;
			ix.ixZero += pName->size[0];
		}
	} while (ispp(','));
}

/*============================================================================
 * Struct Declaration
 *============================================================================*/

void structDeclaration(void)
{
	ix.tix += 2;
	int tagId = cd.token[ix.tix++].ival;
	Name *pName = newName(NM_STRUCT, 0, tagId, -1, 0);
	cd.offset = 0;
	int sizeAlign = 1;
	for (skip('{'); !ispp('}'); skip(';'))
	{
		variableDeclaration(pName, ST_STRUCT);
		if (var.type == ID.SHORT && sizeAlign < 2) sizeAlign = 2;
		if ((var.type == ID.INT || var.type == ID.FLOAT) && sizeAlign < 4) sizeAlign = 4;
		if (var.type == ID.DOUBLE && sizeAlign < 8) sizeAlign = 8;
	}
	if (sizeAlign > 1)
		cd.offset = align(cd.offset, sizeAlign);
	pName->size[0] = cd.offset;
	if (ispp('*')) pName->ptrs = 1;
	int nameId = cd.token[ix.tix++].ival;
	pName->dataType = nameId;
	putName(globTable, pName);
	cd.hash.tbl[nameId].val = (void*)((intptr_t)cd.hash.tbl[nameId].val | AT_TYPE);
	skip(';');
}

/*============================================================================
 * Enum Definition
 *============================================================================*/

void enumDefinition(void)
{
	int val = 0;
	skip(ID.ENUM);
	for (skip('{'); !ispp('}'); ispp(','))
	{
		int hid = cd.token[ix.tix++].ival;
		Name *pName = appendName(globTable, NM_ENUM, 0, hid, -1, val++);
		if (ispp('='))
		{
			val = cd.token[ix.tix++].ival;
			pName->address = val++;
		}
	}
	skip(';');
}

/*============================================================================
 * Parameter Declaration
 *============================================================================*/

void parameterDeclaration(void)
{
	typeSpecifier();
	varDeclarator(TRUE);
	if (ispp('(') && !ispp(')'))
		error("paramDecl", "')' expected");
	Name *pName = appendName(cd.currTable, NM_VAR, var.type, var.id, AD_STACK, cd.baseSpace);
	setAttr(pName);
	cd.baseSpace += var.width;
}

/*============================================================================
 * Function Definition
 *============================================================================*/

void functionDefinition(void)
{
	int callConv = NM_CDECL;
	int fRtnStmt = FALSE;
	int fProtoType = FALSE;
	int ixCodeSave = ix.ixCode;
	int ixSp, n, argc, ptrs;
	PTRS_TYPE argpt[256];

	int fExp = is(ID.DECLSP);
	typeSpecifier();
	if (varDeclarator(FALSE) == NM_WINAPI) callConv = NM_WINAPI;
	int idFunc = var.id;
	createNameTable(var.id);
	outCode2((fExp ? exp_ : fn_), var.id);
	Name *pName = getNameFromTable(globTable, NM_CDECL, var.id);
	if (pName == NULL)
	{
		pName = appendName(globTable, callConv, var.type, var.id, AD_CODE, 0);
		pName->ptrs = var.pointers + var.arrays + getPtr(var.type);
	}
	else if (pName->dataType != var.type || pName->type != callConv)
	{
		error("funcDef", "incompatible types for redefinition of '%s'", toString(var.id));
	}
	cd.baseSpace = 4 * 2;
	outCode1(xent);
	ixSp = ix.ixCode;
	outCode2(sub_esp, 0);
	argc = 0;
	for (skip('('); !ispp(')'); ispp(','))
	{
		int fD3 = ispp(ID.DOTS3);
		if (!fD3)
		{
			parameterDeclaration();
			ptrs = var.pointers + var.arrays + getPtr(var.type);
		}
		argpt[argc].ptrs = fD3 ? 0 : ptrs;
		argpt[argc++].type = fD3 ? ID.DOTS3 : var.type;
	}
	pName->argc = argc;
	int sizeRet = callConv == NM_WINAPI ? (cd.baseSpace - 8) : 0;
	if (argc > 0)
	{
		pName->argpt = calloc(argc, sizeof(PTRS_TYPE));
		memcpy(pName->argpt, argpt, argc * sizeof(PTRS_TYPE));
	}
	cd.baseSpace = 0;
	if (ispp(';')) fProtoType = TRUE;
	else fRtnStmt = compoundStatement(0, 0, sizeRet);
	if (cd.baseSpace < 0)
		cd.baseSpace = -align(-cd.baseSpace, 4);
	if (cd.baseSpace != 0)
		cd.pCode[ixSp].num = -cd.baseSpace;
	else
		memmove(&cd.pCode[ixSp], &cd.pCode[ixSp + 1], (--ix.ixCode - ixSp)*sizeof(INSTRUCT));
	if (!fRtnStmt) outCode2(xret, sizeRet);
	deleteNameTable();
	if (fProtoType) ix.ixCode = ixCodeSave;
	else cd.hash.tbl[idFunc].val += (fExp ? AT_EXPT : AT_USER);
}

/*============================================================================
 * Statements
 *============================================================================*/

static void ifStatement(int locBreak, int locContinue, int sizeRet)
{
	int locElse = loc(), locEnd = -1;
	ix.tix++;
	expr2(VAL);
	jumpFalse(locElse, "if");
	int fReturn = is(ID.RETURN);
	statement(locBreak, locContinue, sizeRet);
	if (is(ID.ELSE) && !fReturn) outCode2(jmp, locEnd = loc());
	outCode3(loc_, locElse, 'E');
	if (ispp(ID.ELSE))
	{
		statement(locBreak, locContinue, sizeRet);
		if (locEnd >= 0) outCode2(loc_, locEnd);
	}
}

static void forStatement(int isFOR, int sizeRet)
{
	int locExpr2 = loc(), locExpr3 = 0, locStmt = loc(), locNext = loc();
	ix.tix++;
	skip('(');
	if (isFOR)
	{
		if (!is(';')) expr(VAL);
		skip(';');
	}
	outCode2(loc_, locExpr2);
	int bgn = ix.ixCode;
	if (!is(';')) expr(VAL);
	else outCode2(mov_eax, 1);
	if (isFOR) skip(';');
	jumpFalse(locNext, "for");
	outCode2(jmp, locStmt);
	if (cd.pCode[bgn].inst == mov_eax && cd.pCode[bgn + 1].inst == test_eax_eax)
	{
		ix.ixCode = bgn;
		outCode2(jmp, cd.pCode[bgn].num == 0 ? locNext : locStmt);
	}
	int bgnExpr = ix.ixCode;
	outCode2(loc_, locExpr3 = loc());
	if (isFOR && !is(')')) expr(VAL);
	skip(')');
	int bgnStmt = ix.ixCode;
	outCode3(loc_, locStmt, '1');
	statement(locNext, locExpr3, sizeRet);
	int endStmt = ix.ixCode;
	int lenStmt = endStmt - bgnStmt;
	int lenBoth = endStmt - bgnExpr;
	if (bgnStmt - bgnExpr > 0 && lenStmt > 0)
	{
		reallocCode(lenStmt);
		memmove(&cd.pCode[bgnExpr + lenStmt], &cd.pCode[bgnExpr], lenBoth*sizeof(INSTRUCT));
		memmove(&cd.pCode[bgnExpr], &cd.pCode[bgnStmt + lenStmt], lenStmt*sizeof(INSTRUCT));
	}
	outCode2(jmp, locExpr2);
	outCode3(loc_, locNext, '0');
}

static void whileStatement(int sizeRet)
{
	forStatement(FALSE, sizeRet);
}

static void doStatement(int sizeRet)
{
	int locStmt = loc(), locBreak = loc();
	ix.tix++;
	outCode2(loc_, locStmt);
	compoundStatement(locBreak, locStmt, sizeRet);
	if (!ispp(ID.WHILE)) error("doStmt", "while expected");
	expr2(VAL);
	skip(';');
	jumpTrue(locStmt, "do");
	outCode2(loc_, locBreak);
}

static void returnStatement(int sizeRet)
{
	ix.tix++;
	if (!is(';'))
	{
		VALUE v;
		expression(VAL, &v);
	}
	skip(';');
	outCode2(xret, sizeRet);
}

static void continueStatement(int locContinue)
{
	if (locContinue <= 0)
		error("", "continue statement not within a loop");
	ix.tix++;
	skip(';');
	outCode2(jmp, locContinue);
}

static void breakStatement(int locBreak)
{
	if (locBreak <= 0)
		error("", "break statement not within loop or switch");
	ix.tix++;
	skip(';');
	outCode2(jmp, locBreak);
}

static void switchStatement(int sizeRet)
{
	int locCase = loc(), locStmt = loc(), locBreak;
	int fDefault = 0;
	ix.tix++;
	expr2(VAL);
	skip('{');
	locBreak = loc();
	while (!ispp('}'))
	{
		if (ispp(ID.CASE))
		{
			outCode2(loc_, locCase);
			outCode2(cmp_eax, constIntExpression());
			skip(':');
			outCode2(jnz, locCase = loc());
			outCode2(loc_, locStmt);
			if (!is(ID.CASE))
			{
				do
				{
					statement(locBreak, -1, sizeRet);
				} while (!is(ID.CASE) && !is(ID.DEFAULT) && !is('}'));
			}
			outCode2(jmp, locStmt = loc());
		}
		else if ((fDefault = ispp(ID.DEFAULT)))
		{
			outCode2(loc_, locCase);
			skip(':');
			outCode2(loc_, locStmt);
			do
			{
				statement(locBreak, -1, sizeRet);
			} while (!is('}'));
		}
		else
		{
			error("switchStmt", "'case' or 'default' expected");
		}
	}
	if (!fDefault)
	{
		outCode2(loc_, locCase);
		outCode2(loc_, locStmt);
	}
	outCode2(loc_, locBreak);
}

void statement(int locBreak, int locContinue, int sizeRet)
{
	if (is(';')) ix.tix++;
	else if (is('{')) compoundStatement(locBreak, locContinue, sizeRet);
	else if (is(ID.IF)) ifStatement(locBreak, locContinue, sizeRet);
	else if (is(ID.FOR)) forStatement(TRUE, sizeRet);
	else if (is(ID.WHILE)) whileStatement(sizeRet);
	else if (is(ID.DO)) doStatement(sizeRet);
	else if (is(ID.RETURN)) returnStatement(sizeRet);
	else if (is(ID.BREAK)) breakStatement(locBreak);
	else if (is(ID.CONTINUE)) continueStatement(locContinue);
	else if (is(ID.SWITCH)) switchStatement(sizeRet);
	else
	{
		expr(VAL);
		skip(';');
	}
}

int compoundStatement(int locBreak, int locContinue, int sizeRet)
{
	createNameTable(-1);
	for (skip('{'); !ispp('}');)
	{
		if (cd.token[ix.tix + 1].ival != '(' && isTypeSpecifier(ix.tix))
		{
			variableDeclaration(NULL, ST_FUNC);
			skip(';');
		}
		else
		{
			statement(locBreak, locContinue, sizeRet);
		}
	}
	deleteNameTable();
	int n = ix.ixCode;
	while (--n > 0 && (cd.pCode[n].inst == loc_ || cd.pCode[n].inst == fn_));
	return cd.pCode[n].inst == xret;
}

/*============================================================================
 * Inicialização do Parser
 *============================================================================*/

typedef struct _Keyword
{
	int *id;
	char *name;
} Keyword;

void init(void)
{
	int n;
	char *op2 = OPERATOR2, s[2] = { 0, '\0' }, s2[3];
	Keyword dataType[] =
	{
		{ &ID.HVOID, "void" }, { &ID.CHAR, "char" }, { &ID.SHORT, "short" },
		{ &ID.INT, "int" }, { &ID.FLOAT, "float" }, { &ID.DOUBLE, "double" },
		{ &ID.STATIC, "static" }, { &ID.STRUCT, "struct" }
	};
	Keyword keyword[] =
	{
		{ &ID.IF, "if" }, { &ID.ELSE, "else" }, { &ID.WHILE, "while" },
		{ &ID.DO, "do" }, { &ID.FOR, "for" },
		{ &ID.RETURN, "return" }, { &ID.CONTINUE, "continue" }, { &ID.BREAK, "break" },
		{ &ID.SWITCH, "switch" }, { &ID.CASE, "case" }, { &ID.DEFAULT, "default" },
		{ &ID.SIZEOF, "sizeof" }, { &ID.DOTS3, "..." },
		{ &ID.TYPEDEF, "typedef" }, { &ID.HWINAPI, "WINAPI" }, { &ID.ENUM, "enum" },
		{ &ID.HCONST, "const" }, { &ID.EXTERN, "extern" }, { &ID.DECLSP, "__declspec" },
		{ &ID.DLLIMPT, "dllimport" }, { &ID.DLLEXPT, "dllexport" }
	};
	for (*s = ' ' + 1; *s < 127; (*s)++)
	{
		if (isgraph(*s) && !isdigit(*s)) put(s, NULL, &cd.hash);
	}
	for (n = 0; op2[n] != '\0'; n += 3)
	{
		sprintf(s2, "%c%c", op2[n], op2[n + 1]);
		if (id2(&op2[n]) != put(s2, NULL, &cd.hash))
			error("parser.init", "Hash size mismatch");
	}
	for (n = 0; n < sizeof(dataType) / sizeof(Keyword); n++)
		*(dataType[n].id) = put(dataType[n].name, (void*)AT_TYPE, &cd.hash);
	for (n = 0; n < sizeof(keyword) / sizeof(Keyword); n++)
		*(keyword[n].id) = put(keyword[n].name, NULL, &cd.hash);
}

/*============================================================================
 * Program
 *============================================================================*/

static int isFunctionDefinition(void)
{
	int n = is(ID.DECLSP) ? 5 : 1;
	if (is(ID.STATIC)) return FALSE;
	if (n == 1 && !isTypeSpecifier(ix.tix)) return FALSE;
	while (ix.tix + n < cd.nToken && cd.token[ix.tix + n].ival == '*')
		n++;
	if (cd.token[ix.tix + n].ival == ID.HWINAPI) return TRUE;
	return cd.token[ix.tix + n + 1].ival == '(';
}

void program(void)
{
	for (ix.tix = 0; ix.tix + 2 < cd.nToken;)
	{
		if (opt&oTRACE) printToken(ix.tix);
		if (is(ID.TYPEDEF) && ix.tix + 5 < cd.nToken
			&& cd.token[ix.tix + 3].ival != '{')
		{
			ix.tix += 5;
		}
		else if (is(ID.TYPEDEF))
		{
			structDeclaration();
		}
		else if (is(ID.ENUM))
		{
			enumDefinition();
		}
		else if (isFunctionDefinition())
		{
			functionDefinition();
		}
		else
		{
			variableDeclaration(NULL, ST_GVAR);
			skip(';');
		}
	}
}

/*============================================================================
 * Parse Principal
 *============================================================================*/

void parse(void)
{
	int n;
	cd.sizeCode = 10000;
	cd.pCode = xalloc(cd.sizeCode * sizeof(INSTRUCT));
	init();
	for (n = 0; n < cd.nToken; n++)
	{
		setToken(&cd.token[n], &cd.hash);
		if (opt&oTOKEN) printToken(n);
	}
	cd.currTable = -1;
	createNameTable(id("<global>"));
	program();
	for (n = 0; n < ix.ixCode - 2; n++)
	{
		int inst = cd.pCode[n].inst;
		int num = cd.pCode[n].num;
		if ((inst == jge || inst == jl || inst == jz || inst == jnz)
			&& cd.pCode[n + 1].inst == jmp && cd.pCode[n + 2].num == num)
		{
			cd.pCode[n].num = cd.pCode[n + 1].num;
			cd.pCode[n + 1].num = num;
			cd.pCode[n].inst = inst == jge ? jl : inst == jl ? jge : inst == jz ? jnz : jz;
		}
	}
	for (n = 0; n < ix.ixCode - 1; n++)
	{
		if (cd.pCode[n].inst == jmp && cd.pCode[n + 1].inst == loc_
			&& cd.pCode[n].num == cd.pCode[n + 1].num) delCode(n);
	}
}

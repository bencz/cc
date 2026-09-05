/* C declarations, statements, types, and name resolution. */

#include "cc.h"

static char *toStr(int tix);
static int isId(int offset);
static void createNameTable(int functionId);
static void deleteNameTable(void);
static Name *newName(int type, int dataType, int name, int addressType, int address);
static Name *putName(int blockIndex, Name *name);
static Name *getStruct(int type);
static Name *getMember(int type, int index);
static void constExpression(VALUE *value);
static int constIntExpression(void);
static int align(int position, int width);
static int varDeclarator(int parameter);
static void initializer(Name *name, int addressType, int depth, int address);
static void variableDeclaration(Name *structure, int status);
static void structDeclaration(void);
static void enumDefinition(void);
static Name *parameterDeclaration(void);
static void functionDefinition(void);

typedef struct _StatementContext
{
	int breakLocation;
	int continueLocation;
	int returnSize;
	IrBlockId breakBlock;
	IrBlockId continueBlock;
} StatementContext;

static void statement(const StatementContext *context);
static int compoundStatement(const StatementContext *context);
static void init(void);
static void program(void);

/* Token helpers */

static char *toStr(int tix)
{
	if (tix < 0 || tix >= cd.nToken)
	{
		error("parser", "token index %d is outside the token stream", tix);
	}
	return cd.token[tix].token;
}

char *toString(int id)
{
	if (id < 0 || id >= cd.hash.size || cd.hash.tbl[id].state != 1U)
	{
		error("parser", "invalid identifier-table index %d", id);
	}
	return cd.hash.tbl[id].key;
}

int id2(char *op)
{
	return (op[0] * 137 + op[1]) % cd.hash.size;
}

static int isId(int n)
{
	if (ix.tix + n < 0 || ix.tix + n >= cd.nToken)
	{
		return FALSE;
	}
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

int id(char *str)
{
	return hashPut(str, NULL, &cd.hash);
}

int ispp(int id)
{
	if (is(id))
	{
		ix.tix++;
		return 1;
	}
	else
	{
		return 0;
	}
}

int is2pp(char *op)
{
	if (is2(op))
	{
		ix.tix++;
		return 1;
	}
	else
	{
		return 0;
	}
}

void skip(int id)
{
	if (!ispp(id))
	{
		error("code.skip", "'%s' expected before '%s'", toString(id), toStr(ix.tix));
	}
}

/* Type helpers */

int canonicalType(int type)
{
	Name *alias = getStruct(type);
	while (alias != NULL && (alias->type & NM_TYPEDEF) != 0)
	{
		type = alias->idName;
		alias = getStruct(type);
	}
	return type;
}

int isFloatingType(int type)
{
	type = canonicalType(type);
	return type == ID.T_FLOAT || type == ID.T_DOUBLE;
}

int arithmeticType(int left, int right)
{
	left = canonicalType(left);
	right = canonicalType(right);
	if (left == ID.T_DOUBLE || right == ID.T_DOUBLE)
	{
		return ID.T_DOUBLE;
	}
	if (left == ID.T_FLOAT || right == ID.T_FLOAT)
	{
		return ID.T_FLOAT;
	}
	return usualIntegerType(left, right);
}

int sizeOfDataType(int type)
{
	Name *alias = getStruct(type);
	if (alias != NULL && (alias->type & NM_TYPEDEF) != 0 && alias->arrays > 0)
	{
		return alias->size[0];
	}
	type = canonicalType(type);
	if (type == ID.T_BOOL || type == ID.T_CHAR || type == ID.T_SCHAR || type == ID.T_UCHAR ||
	    type == ID.T_VOID)
	{
		return 1;
	}
	if (type == ID.T_SHORT || type == ID.T_USHORT)
	{
		return cmd.target->dataLayout.shortSize;
	}
	if (type == ID.T_INT || type == ID.T_UINT)
	{
		return cmd.target->dataLayout.intSize;
	}
	if (type == ID.T_LONG || type == ID.T_ULONG)
	{
		return cmd.target->dataLayout.longSize;
	}
	if (type == ID.T_FLOAT)
	{
		return cmd.target->dataLayout.floatSize;
	}
	if (type == ID.T_DOUBLE)
	{
		return cmd.target->dataLayout.doubleSize;
	}
	Name *pName = getStruct(type);
	return pName != NULL ? pName->size[0] : 0;
}

int sizeOfPointer(void)
{
	return cmd.target->dataLayout.pointerSize;
}

int sizeOfObjectType(int type, int pointers)
{
	return pointers > 0 ? sizeOfPointer() : sizeOfDataType(type);
}

int alignmentOfObjectType(int type, int pointers)
{
	Name *aggregate;
	int canonical;

	if (pointers > 0)
	{
		return cmd.target->dataLayout.pointerAlignment;
	}
	canonical = canonicalType(type);
	aggregate = getStruct(canonical);
	if (aggregate != NULL)
	{
		return aggregate->alignment > 0 ? aggregate->alignment : 1;
	}
	return sizeOfDataType(canonical);
}

IrType irTypeForCObject(int type, int pointers)
{
	int canonical = canonicalType(type);
	unsigned int size;
	if (pointers > 0)
	{
		return irTypePointer((unsigned int)cmd.target->dataLayout.pointerSize * CHAR_BIT,
		                     cmd.target->dataLayout.pointerAlignment);
	}
	if (canonical == ID.T_VOID)
	{
		return irTypeVoid();
	}
	size = (unsigned int)sizeOfDataType(canonical);
	if (canonical == ID.T_FLOAT || canonical == ID.T_DOUBLE)
	{
		return irTypeFloat(size * CHAR_BIT, size);
	}
	if (isIntegerType(canonical))
	{
		return irTypeInteger(size * CHAR_BIT, isUnsignedType(canonical), size);
	}
	if (getStruct(canonical) != NULL && size <= sizeof(int))
	{
		IrType aggregate =
		    irTypeInteger(size * CHAR_BIT, TRUE, (unsigned int)alignmentOfObjectType(canonical, 0));
		aggregate.kind = IR_TYPE_AGGREGATE;
		return aggregate;
	}
	error("parser", "type '%s' (size %u) is not a scalar IR type", toString(canonical), size);
}

int isIntegerType(int type)
{
	type = canonicalType(type);
	return type == ID.T_BOOL || type == ID.T_CHAR || type == ID.T_SCHAR || type == ID.T_UCHAR ||
	       type == ID.T_SHORT || type == ID.T_USHORT || type == ID.T_INT || type == ID.T_UINT ||
	       type == ID.T_LONG || type == ID.T_ULONG;
}

int isUnsignedType(int type)
{
	type = canonicalType(type);
	return type == ID.T_BOOL || type == ID.T_UCHAR || type == ID.T_USHORT || type == ID.T_UINT ||
	       type == ID.T_ULONG || (type == ID.T_CHAR && cmd.target->dataLayout.plainCharUnsigned);
}

int integerPromotion(int type)
{
	type = canonicalType(type);
	if (type == ID.T_BOOL || type == ID.T_CHAR || type == ID.T_SCHAR || type == ID.T_UCHAR ||
	    type == ID.T_SHORT || type == ID.T_USHORT)
	{
		return ID.T_INT;
	}
	return type;
}

static int integerRank(int type)
{
	type = integerPromotion(type);
	return type == ID.T_LONG || type == ID.T_ULONG ? 2 : 1;
}

int usualIntegerType(int leftType, int rightType)
{
	int left = integerPromotion(leftType);
	int right = integerPromotion(rightType);
	int rank = integerRank(left) > integerRank(right) ? integerRank(left) : integerRank(right);
	int makeUnsigned = isUnsignedType(left) || isUnsignedType(right);
	if (rank == 2)
	{
		return makeUnsigned ? ID.T_ULONG : ID.T_LONG;
	}
	return makeUnsigned ? ID.T_UINT : ID.T_INT;
}

int isTypeSpecifier(int tokenIndex)
{
	int hix = cd.token[tokenIndex].ival;
	return cd.token[tokenIndex].type == TK_NAME &&
	       ((((intptr_t)cd.hash.tbl[hix].val & AT_TYPE) != 0) || hix == ID.HCONST ||
	        hix == ID.SIGNED || hix == ID.UNSIGNED || hix == ID.HLONG || hix == ID.ENUM);
}

/* Name tables */

static int cmpSeq(const void *a, const void *b)
{
	return ((HDATA *)a)->seq - ((HDATA *)b)->seq;
}

static void formatNameKey(char *key, size_t capacity, int type, const char *name)
{
	const char *prefix = (type & NM_FUNC) ? "@" : (type & NM_STRUCT) ? "$" : "";
	int written = snprintf(key, capacity, "%s%s", prefix, name);
	if (written < 0 || (size_t)written >= capacity)
	{
		error("parser", "identifier is too long for the name table: %s", name);
	}
}

void printNameTable(int ixBlk)
{
	Block *blk = &cd.block[ixBlk];
	printf("------ %-16s#%d [%2d]-------\n", toString(blk->idFunc), blk->blockDepth, ixBlk);
	printf("%25s  %-25s %s\n", "dtype", "name", "ptrs atype   addr    size");
	qsort(blk->hash.tbl, blk->hash.size, sizeof(HDATA), cmpSeq);
	int n;
	for (n = 0; n < blk->hash.size; n++)
	{
		Name *e = blk->hash.tbl[n].val;
		if (e == NULL)
		{
			continue;
		}
		char *fmt = strlen(toString(e->dataType)) < 26 ? "%25s  %-25s" : "%.23s..  %.23s..";
		printf(fmt, toString(e->dataType), toString(e->idName));
		printf("   %d%5d%9d%8d\n", e->ptrs, e->addrType, e->address, e->size[0]);
	}
}

static void createNameTable(int idFunc)
{
	if (++cd.currTable > 1)
	{
		idFunc = cd.block[cd.currTable - 1].idFunc;
	}
	cd.block[cd.currTable].idFunc = idFunc;
	cd.block[cd.currTable].blockDepth = cd.currTable;
	hashInit('n', cd.currTable == 0 ? 1000 : 4, &cd.block[cd.currTable].hash);
}

static void deleteNameTable(void)
{
	int index;
	HASH *names = &cd.block[cd.currTable].hash;
	if (opt & oNAME)
	{
		printNameTable(cd.currTable);
	}
	for (index = 0; index < names->size; ++index)
	{
		if (names->tbl[index].state == 1U && names->tbl[index].val != NULL)
		{
			Name *name = names->tbl[index].val;
			free(name->argpt);
		}
	}
	hashFree(&cd.block[cd.currTable--].hash);
}

static Name *newName(int type, int dataType, int name, int addrType, int address)
{
	Name *pNew = xalloc(sizeof(Name));
	pNew->type = type;
	pNew->idName = name;
	pNew->dataType = dataType;
	pNew->addrType = addrType;
	pNew->address = address;
	pNew->argc = -1;
	pNew->irLocal = IR_LOCAL_NONE;
	pNew->irSymbol = addrType == AD_STACK ? IR_SYMBOL_NONE : name;
	return pNew;
}

static Name *putName(int ixBlk, Name *pName)
{
	char key[128];
	int type = pName->type;
	char *name = toString((type & NM_STRUCT) ? pName->dataType : pName->idName);
	formatNameKey(key, sizeof(key), type, name);
	hashPut(key, pName, &cd.block[ixBlk].hash);
	return pName;
}

Name *appendName(int ixBlk, int type, int dataType, int name, int addrType, int address)
{
	return putName(ixBlk, newName(type, dataType, name, addrType, address));
}

Name *getNameFromTable(int ixBlk, int type, int name)
{
	char key[128];
	formatNameKey(key, sizeof(key), type, toString(name));
	return hashGet(key, &cd.block[ixBlk].hash);
}

Name *getNameFromAllTable(int ixBlk, int type, int name)
{
	for (; ixBlk >= 0; ixBlk--)
	{
		Name *e = getNameFromTable(ixBlk, type, name);
		if (e != NULL)
		{
			return e;
		}
	}
	return NULL;
}

static Name *getStruct(int type)
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
	Name *aggregate = getStruct(canonicalType(type));
	Name *e;
	if (aggregate == NULL)
	{
		return NULL;
	}
	e = aggregate->pBgn;
	while (e != NULL && e->idName != name)
	{
		e = e->pNext;
	}
	return e;
}

static Name *getMember(int type, int n)
{
	Name *aggregate = getStruct(canonicalType(type));
	Name *e;
	if (aggregate == NULL)
	{
		return NULL;
	}
	e = aggregate->pBgn;
	while (n-- > 0 && e != NULL)
	{
		e = e->pNext;
	}
	return e;
}

/* Constant expressions */

static void constExpression(VALUE *pv)
{
	int ixCodeSave = ix.ixCode;
	int ixDataSave = ix.ixData;
	irBuilderSuspend(&compiler.irBuilder);
	conditionalExpression(VAL, pv);
	irBuilderResume(&compiler.irBuilder);
	ix.ixCode = ixCodeSave;
	ix.ixData = ixDataSave;
}

static int constIntExpression(void)
{
	VALUE v;
	constExpression(&v);
	if (!v.fConst || !isIntegerType(v.type))
	{
		error("parser", "constant expression expected");
	}
	return v.ival;
}

/* Alignment */

static int align(int pos, int width)
{
	if (pos == 0)
	{
		return 0;
	}
	if ((width & 7) == 0)
	{
		return (pos + 7) & ~7;
	}
	if ((width & 3) == 0)
	{
		return (pos + 3) & ~3;
	}
	if ((width & 1) == 0)
	{
		return (pos + 1) & ~1;
	}
	return pos;
}

/*============================================================================
 * Type Specifier
 *============================================================================*/

void typeSpecifier(void)
{
	int sign = 0;
	int longCount = 0;
	int base = 0;
	int shortSeen = FALSE;
	int consumedBuiltin = FALSE;

	if (is(ID.DECLSP))
	{
		ix.tix += 4;
	}
	for (;;)
	{
		if (ispp(ID.HCONST))
		{
			continue;
		}
		if (ispp(ID.SIGNED))
		{
			if (sign != 0)
			{
				error("typeSpecifier", "duplicate or conflicting signedness specifier");
			}
			sign = 1;
			consumedBuiltin = TRUE;
			continue;
		}
		if (ispp(ID.UNSIGNED))
		{
			if (sign != 0)
			{
				error("typeSpecifier", "duplicate or conflicting signedness specifier");
			}
			sign = 2;
			consumedBuiltin = TRUE;
			continue;
		}
		if (ispp(ID.HLONG))
		{
			++longCount;
			consumedBuiltin = TRUE;
			continue;
		}
		if (ispp(ID.T_SHORT))
		{
			if (shortSeen)
			{
				error("typeSpecifier", "duplicate 'short' specifier");
			}
			shortSeen = TRUE;
			consumedBuiltin = TRUE;
			continue;
		}
		if (is(ID.T_BOOL) || is(ID.T_CHAR) || is(ID.T_INT) || is(ID.T_DOUBLE) || is(ID.T_FLOAT))
		{
			if (base != 0)
			{
				error("typeSpecifier", "conflicting type specifiers");
			}
			base = cd.token[ix.tix++].ival;
			consumedBuiltin = TRUE;
			continue;
		}
		break;
	}
	if (longCount > 1)
	{
		error("typeSpecifier", "64-bit 'long long' is not supported by the 32-bit IR");
	}
	if (!consumedBuiltin)
	{
		int fAggregate = is(ID.STRUCT) || is(ID.UNION) || is(ID.ENUM);
		if (fAggregate || is(ID.TYPEDEF))
		{
			ix.tix++;
		}
		if (!fAggregate && !isTypeSpecifier(ix.tix))
		{
			error("typeSpecifier", "'%s' undeclared", toStr(ix.tix));
		}
		var.type = cd.token[ix.tix++].ival;
		return;
	}
	if (base == 0)
	{
		base = ID.T_INT;
	}
	if (shortSeen)
	{
		if (base != ID.T_INT || longCount != 0)
		{
			error("typeSpecifier", "invalid use of 'short' in type specifier");
		}
		var.type = sign == 2 ? ID.T_USHORT : ID.T_SHORT;
		return;
	}
	if (base == ID.T_BOOL)
	{
		if (sign != 0 || longCount != 0)
		{
			error("typeSpecifier", "invalid type specifier combined with '_Bool'");
		}
		var.type = ID.T_BOOL;
	}
	else if (base == ID.T_CHAR)
	{
		if (longCount != 0)
		{
			error("typeSpecifier", "invalid use of 'long' with 'char'");
		}
		var.type = sign == 2 ? ID.T_UCHAR : sign == 1 ? ID.T_SCHAR : ID.T_CHAR;
	}
	else if (base == ID.T_INT)
	{
		var.type = longCount != 0 ? (sign == 2 ? ID.T_ULONG : ID.T_LONG)
		                          : (sign == 2 ? ID.T_UINT : ID.T_INT);
	}
	else if (base == ID.T_DOUBLE && longCount != 0)
	{
		var.type = ID.T_DOUBLE;
	}
	else
	{
		if (sign != 0 || (longCount != 0 && base != ID.T_DOUBLE))
		{
			error("typeSpecifier", "invalid type specifier combination");
		}
		var.type = base;
	}
}

static Name *defineTypeAlias(int aliasId, int underlyingType, int pointers, int size)
{
	Name *alias;
	if (aliasId == ID.T_INT || aliasId == ID.T_CHAR || aliasId == ID.T_SHORT ||
	    aliasId == ID.T_FLOAT || aliasId == ID.T_DOUBLE || aliasId == ID.T_VOID ||
	    aliasId == ID.T_BOOL)
	{
		error("typedef", "reserved type name cannot be redefined: %s", toString(aliasId));
	}
	if (getStruct(aliasId) != NULL)
	{
		error("typedef", "redefinition of typedef '%s'", toString(aliasId));
	}
	alias = newName(NM_STRUCT | NM_TYPEDEF, aliasId, underlyingType, 0, 0);
	alias->dataType = aliasId;
	alias->ptrs = pointers;
	alias->size[0] = size;
	alias->alignment = alignmentOfObjectType(underlyingType, pointers);
	putName(globTable, alias);
	cd.hash.tbl[aliasId].val = (void *)((intptr_t)cd.hash.tbl[aliasId].val | AT_TYPE);
	return alias;
}

static void typedefDeclaration(void)
{
	int underlyingType;
	skip(ID.TYPEDEF);
	typeSpecifier();
	underlyingType = var.type;
	do
	{
		Name *alias;
		int dimension;
		var.type = underlyingType;
		varDeclarator(FALSE);
		if (var.id < 0)
		{
			error("typedef", "typedef name expected");
		}
		alias = defineTypeAlias(
		    var.id, var.type, var.pointers + getPtr(var.type), var.width * var.length);
		alias->arrays = var.arrays;
		alias->size[var.arrays] = var.width;
		for (dimension = var.arrays - 1; dimension >= 0; --dimension)
		{
			alias->size[dimension] = var.size[dimension] * alias->size[dimension + 1];
		}
	} while (ispp(','));
	skip(';');
}

/*============================================================================
 * Variable Declarator
 *============================================================================*/

static void setAttr(Name *pName)
{
	int n = var.size[0] == 0 ? 0 : var.arrays;
	pName->ptrs = var.pointers + var.arrays;
	pName->arrays = var.arrays;
	pName->alignment = var.alignment;
	pName->size[n] = var.width;
	while (--n >= 0)
	{
		pName->size[n] = pName->size[n + 1] * var.size[n];
	}
	if (var.isFunctionPointer)
	{
		pName->isFunctionPointer = TRUE;
		pName->returnPointers = var.returnPointers;
		pName->functionCallConvention = var.functionCallConvention;
		pName->argc = var.functionParameterCount;
		if (pName->argc > 0)
		{
			pName->argpt = xalloc((size_t)pName->argc * sizeof(*pName->argpt));
			memcpy(
			    pName->argpt, var.functionParameters, (size_t)pName->argc * sizeof(*pName->argpt));
		}
	}
}

static void countMember(int depth, int *dim)
{
	VALUE v;
	int n = 0;
	while (!is('}'))
	{
		if (ispp('{'))
		{
			countMember(depth + 1, dim);
		}
		else
		{
			assignExpression(VAL, &v);
		}
		++n;
		if (!ispp(','))
		{
			break;
		}
	}
	skip('}');
	if (n > dim[depth])
	{
		dim[depth] = n;
	}
}

static int varDeclarator(int fParam)
{
	int n, fCount = FALSE, callConv = NM_CDECL;
	int returnPointers;
	int declaratorType = var.type;

	var.isFunctionPointer = FALSE;
	var.returnPointers = 0;
	var.functionCallConvention = NM_CDECL;
	var.functionParameterCount = 0;

	for (var.pointers = 0; ix.tix < cd.nToken && ispp('*');)
	{
		var.pointers++;
	}
	returnPointers = var.pointers;
	if (ispp(ID.HWINAPI))
	{
		callConv = NM_WINAPI;
	}
	if (ispp('('))
	{
		int pointerDepth = 0;
		int parameterCount = 0;
		int declaredId;
		PTRS_TYPE parameters[64];

		while (ispp('*'))
		{
			++pointerDepth;
		}
		if (pointerDepth == 0)
		{
			error("declarator", "'*' expected in parenthesized declarator");
		}
		if (pointerDepth != 1)
		{
			error("declarator", "pointers to function pointers are not supported");
		}
		if (ispp(ID.HWINAPI))
		{
			callConv = NM_WINAPI;
		}
		if (cd.token[ix.tix].type == TK_SYMBOL)
		{
			var.id = -1;
		}
		else
		{
			var.id = cd.token[ix.tix++].ival;
		}
		declaredId = var.id;
		skip(')');
		skip('(');
		if (is(ID.T_VOID) && isN(')', 1))
		{
			ix.tix += 2;
		}
		else
		{
			while (!ispp(')'))
			{
				if (parameterCount >= (int)(sizeof(parameters) / sizeof(parameters[0])))
				{
					error("declarator", "function pointer has too many parameters");
				}
				if (ispp(ID.DOTS3))
				{
					parameters[parameterCount].type = ID.DOTS3;
					parameters[parameterCount].ptrs = 0;
					++parameterCount;
					if (!is(')'))
					{
						error("declarator", "ellipsis must be the final parameter");
					}
				}
				else
				{
					int parameterType;
					int parameterPointers;

					typeSpecifier();
					parameterType = var.type;
					varDeclarator(TRUE);
					parameterPointers = var.pointers + getPtr(parameterType);
					parameters[parameterCount].type = parameterType;
					parameters[parameterCount].ptrs = parameterPointers;
					++parameterCount;
				}
				if (!ispp(','))
				{
					skip(')');
					break;
				}
			}
		}
		var.type = declaratorType;
		var.id = declaredId;
		var.pointers = pointerDepth;
		var.arrays = 0;
		var.isFunctionPointer = TRUE;
		var.returnPointers = returnPointers;
		var.functionCallConvention = callConv;
		var.functionParameterCount = parameterCount;
		memcpy(var.functionParameters, parameters, (size_t)parameterCount * sizeof(parameters[0]));
		memset(var.size, 0, sizeof(var.size));
		var.width = sizeOfPointer();
		var.alignment = cmd.target->dataLayout.pointerAlignment;
		var.length = 1;
		return callConv;
	}
	if (cd.token[ix.tix].type == TK_SYMBOL)
	{
		var.id = -1;
	}
	else
	{
		var.id = cd.token[ix.tix++].ival;
	}
	memset(&var.size, 0, sizeof(var.size));
	for (var.arrays = 0; ix.tix < cd.nToken && ispp('['); var.arrays++)
	{
		if (var.arrays >= 7)
		{
			error("declarator", "array rank exceeds seven dimensions");
		}
		if (is(']'))
		{
			if (!fParam)
			{
				fCount = TRUE;
			}
		}
		else
		{
			Variable declaration;
			int extent;
			memcpy(&declaration, &var, sizeof(var));
			extent = constIntExpression();
			memcpy(&var, &declaration, sizeof(var));
			if (extent <= 0)
			{
				error("declarator", "array extent must be positive");
			}
			var.size[var.arrays] = extent;
		}
		skip(']');
	}
	if (is('=') && fCount)
	{
		INDEX ixSave;
		Variable varSave;
		int inferredSize[8];
		memcpy(&ixSave, &ix, sizeof(ix));
		memcpy(&varSave, &var, sizeof(var));
		memset(var.size, 0, sizeof(var.size));
		if (cd.token[ix.tix + 1].type == TK_STRING)
		{
			var.size[0] = decodedStringLength(toString(cd.token[ix.tix + 1].ival)) + 1;
		}
		else
		{
			ix.tix += 2;
			countMember(0, var.size);
		}
		memcpy(inferredSize, var.size, sizeof(inferredSize));
		memcpy(&var, &varSave, sizeof(var));
		memcpy(var.size, inferredSize, sizeof(var.size));
		memcpy(&ix, &ixSave, sizeof(ix));
	}
	{
		Name *alias = getStruct(var.type);
		if (alias != NULL && (alias->type & NM_TYPEDEF) != 0 && alias->arrays > 0)
		{
			int dimension;
			if (var.arrays + alias->arrays >= 8 || var.pointers != 0)
			{
				error("declarator", "unsupported pointer-to-array or excessive array rank");
			}
			for (dimension = 0; dimension < alias->arrays; ++dimension)
			{
				var.size[var.arrays++] = alias->size[dimension] / alias->size[dimension + 1];
			}
			var.type = alias->idName;
			var.pointers = alias->ptrs;
		}
	}
	if (fParam)
	{
		var.pointers += var.arrays;
		var.arrays = 0;
	}
	var.width = sizeOfObjectType(var.type, var.pointers + getPtr(var.type));
	var.alignment = alignmentOfObjectType(var.type, var.pointers + getPtr(var.type));
	var.length = 1;
	for (n = 0; n < var.arrays; n++)
	{
		var.length *= var.size[n];
	}
	return callConv;
}

/*============================================================================
 * Initializer
 *============================================================================*/

static Name *findStackStorage(int address)
{
	int tableIndex;
	for (tableIndex = cd.currTable; tableIndex > globTable; --tableIndex)
	{
		HASH *names = &cd.block[tableIndex].hash;
		int slot;
		for (slot = 0; slot < names->size; ++slot)
		{
			Name *candidate = names->tbl[slot].val;
			if (candidate != NULL && candidate->addrType == AD_STACK &&
			    candidate->irLocal != IR_LOCAL_NONE && address >= candidate->address &&
			    address < candidate->address + candidate->size[0])
			{
				return candidate;
			}
		}
	}
	return NULL;
}

static Name *findStaticStorage(int addressType, int address)
{
	int tableIndex;
	for (tableIndex = cd.currTable; tableIndex >= globTable; --tableIndex)
	{
		HASH *names = &cd.block[tableIndex].hash;
		int slot;
		for (slot = 0; slot < names->size; ++slot)
		{
			Name *candidate = names->tbl[slot].val;
			if (candidate != NULL && (candidate->type & NM_VAR) != 0 &&
			    candidate->addrType == addressType && candidate->irSymbol != IR_SYMBOL_NONE &&
			    address >= candidate->address && address < candidate->address + candidate->size[0])
			{
				return candidate;
			}
		}
	}
	return NULL;
}

static void writeStaticBytes(int addressType, int address, const unsigned char *bytes, size_t size)
{
	Name *storage = findStaticStorage(addressType, address);
	IrGlobal *global;
	size_t offset;
	if (storage == NULL)
	{
		error("initializer", "static initializer has no IR storage");
	}
	global = irFindGlobal(&compiler.ir, storage->irSymbol);
	if (global == NULL || global->initializer == NULL)
	{
		error("initializer", "static initializer storage is not initialized");
	}
	offset = (size_t)(address - storage->address);
	if (offset > global->initializerSize || global->initializerSize - offset < size)
	{
		error("initializer", "static initializer exceeds its object");
	}
	memcpy(global->initializer + offset, bytes, size);
}

static void writeStaticInteger(int addressType, int address, uint32_t value, size_t size)
{
	unsigned char bytes[4];
	size_t index;
	if (size == 0U || size > sizeof(bytes))
	{
		error("initializer", "invalid integer initializer width");
	}
	for (index = 0; index < size; ++index)
	{
		size_t shiftIndex = cmd.target->dataLayout.littleEndian ? index : size - index - 1U;
		bytes[index] = (unsigned char)(value >> (shiftIndex * CHAR_BIT));
	}
	writeStaticBytes(addressType, address, bytes, size);
}

static void writeStaticFloat(int addressType, int address, double value, int width)
{
	unsigned char bytes[sizeof(double)];
	size_t size = (size_t)width;
	unsigned int byteOrderProbe = 1U;
	int hostLittleEndian = *(unsigned char *)&byteOrderProbe != 0;
	size_t index;
	if (width == 4)
	{
		float single = (float)value;
		memcpy(bytes, &single, size);
	}
	else
	{
		memcpy(bytes, &value, size);
	}
	if (hostLittleEndian != cmd.target->dataLayout.littleEndian)
	{
		for (index = 0; index < size / 2U; ++index)
		{
			unsigned char temporary = bytes[index];
			bytes[index] = bytes[size - index - 1U];
			bytes[size - index - 1U] = temporary;
		}
	}
	writeStaticBytes(addressType, address, bytes, size);
}

static void addStaticRelocation(int addressType, int address, IrSymbolId symbol, int addend)
{
	Name *storage = findStaticStorage(addressType, address);
	IrGlobal *global;
	if (storage == NULL)
	{
		error("initializer", "static relocation has no IR storage");
	}
	global = irFindGlobal(&compiler.ir, storage->irSymbol);
	if (global == NULL)
	{
		error("initializer", "static relocation references unknown storage");
	}
	irAddGlobalRelocation(global, (size_t)(address - storage->address), symbol, addend);
}

static void initMember(Name *pName, int atype, int depth, int addr)
{
	VALUE v1, v2;
	setValue(VAL, pName->ptrs, pName->dataType, &v1);
	if (atype == AD_STACK)
	{
		Name *storage = findStackStorage(addr);
		int left_bgn = ix.ixCode;
		loadAddr(AD_STACK, addr);
		if (storage != NULL)
		{
			semanticIrLocalAddress(&v1, storage->irLocal, addr - storage->address);
		}
		int left_end = ix.ixCode;
		outCode1(push_eax);
		assignExpression(VAL, &v2);
		infixOperation('=', &v1, &v2, left_bgn, left_end);
	}
	else
	{
		if (!pName->isFunctionPointer && v1.ptrs - depth == 1 &&
		    (v1.type == ID.T_CHAR || v1.type == ID.T_SCHAR || v1.type == ID.T_UCHAR))
		{
			if (cd.token[ix.tix].type == TK_STRING)
			{
				int ival = cd.token[ix.tix++].ival;
				IrSymbolId stringSymbol = semanticIrCreateStringGlobal(toString(ival));

				outDataAddr(ix.ixString);
				outString(ix.ixString, toString(ival));
				ix.ixString += decodedStringLength(toString(ival)) + 1;
				addStaticRelocation(atype, addr, stringSymbol, 0);
			}
			else
			{
				constExpression(&v2);
				outDataInt(0);
				if (v2.constantSymbol != IR_SYMBOL_NONE)
				{
					addStaticRelocation(atype, addr, v2.constantSymbol, v2.constantOffset);
				}
				else if (v2.fConst && v2.ival == 0 &&
				         (isIntegerType(v2.type) || (v2.ptrs > 0 && v2.type == ID.T_VOID)))
				{
					writeStaticInteger(atype, addr, 0U, (size_t)sizeOfPointer());
				}
				else
				{
					error("initMember", "pointer initializer requires an address or null constant");
				}
			}
		}
		else
		{
			constExpression(&v2);
			if (v2.constantSymbol != IR_SYMBOL_NONE)
			{
				if (v1.ptrs - depth <= 0)
				{
					error("initMember", "address constant requires a pointer destination");
				}
				outDataInt(0);
				addStaticRelocation(atype, addr, v2.constantSymbol, v2.constantOffset);
			}
			else if (!v2.fConst)
			{
				error("initMember", "constant expression expected");
			}
			else if (isFloatingType(v1.type))
			{
				double value = isFloatingType(v2.type)   ? v2.rval
				               : isUnsignedType(v2.type) ? (double)(uint32_t)v2.ival
				                                         : (double)v2.ival;
				int width = sizeOfDataType(v1.type);

				if (width == 8)
				{
					outDataDouble(value);
				}
				else
				{
					outDataInt(0);
				}
				writeStaticFloat(atype, addr, value, width);
			}
			else if (v1.ptrs - depth > 0)
			{
				if ((!isIntegerType(v2.type) && !(v2.ptrs > 0 && v2.type == ID.T_VOID)) ||
				    v2.ival != 0)
				{
					error("initMember", "pointer initializer is not a null pointer constant");
				}
				outDataInt(0);
				writeStaticInteger(atype, addr, 0U, (size_t)sizeOfPointer());
			}
			else if (!isIntegerType(v2.type))
			{
				error("initMember", "type mismatch");
			}
			else if (sizeOfDataType(v1.type) == 4)
			{
				outDataInt(v2.ival);
				writeStaticInteger(atype, addr, (uint32_t)v2.ival, 4U);
			}
			else if (sizeOfDataType(v1.type) == 2)
			{
				outDataShort(v2.ival);
				writeStaticInteger(atype, addr, (uint32_t)v2.ival, 2U);
			}
			else if (sizeOfDataType(v1.type) == 1)
			{
				int value = v1.type == ID.T_BOOL ? !!v2.ival : v2.ival;

				outDataChar(value);
				writeStaticInteger(atype, addr, (uint32_t)value, 1U);
			}
			else
			{
				error("initMember", "v1.type=%d", v1.type);
			}
		}
	}
}

static void clear(int bgn, int end)
{
	Name *storage = findStackStorage(bgn);
	if (storage != NULL)
	{
		semanticIrZeroMemory(storage->irLocal, bgn - storage->address, end - bgn);
	}
	outCode2(push, end - bgn);
	outCode2(push, 0);
	outCode2(lea_eax_pbp, bgn);
	outCode1(push_eax);
	outCode2(call, id("memset"));
	outCode2(add_esp, 12);
}

static int memberIndex(int aggregateType, int memberId, Name **member)
{
	Name *aggregate = getStruct(aggregateType);
	Name *candidate;
	int index = 0;
	if (aggregate == NULL)
	{
		error("initializer", "type '%s' is not an aggregate", toString(aggregateType));
	}
	candidate = aggregate->pBgn;
	while (candidate != NULL && candidate->idName != memberId)
	{
		candidate = candidate->pNext;
		++index;
	}
	if (candidate == NULL)
	{
		error("initializer",
		      "'%s' is not a member of '%s'",
		      toString(memberId),
		      toString(aggregateType));
	}
	*member = candidate;
	return index;
}

static void initializer(Name *pName, int atype, int depth, int addr)
{
	int n;
	int fArray = (pName->size[depth + 1] > 0);

	if (!ispp('{'))
	{
		if (atype == AD_DATA)
		{
			ix.ixData = addr;
		}
		if (fArray &&
		    (pName->dataType == ID.T_CHAR || pName->dataType == ID.T_SCHAR ||
		     pName->dataType == ID.T_UCHAR) &&
		    cd.token[ix.tix].type == TK_STRING)
		{
			int ival = cd.token[ix.tix++].ival;
			size_t executionSize;
			unsigned char *executionBytes =
			    semanticIrDecodeExecutionString(toString(ival), &executionSize);

			if (executionSize - 1U > (size_t)pName->size[depth])
			{
				free(executionBytes);
				error("init'r", "initializer-string too long");
			}
			if (atype != AD_STACK)
			{
				size_t storedSize = executionSize > (size_t)pName->size[depth]
				                        ? (size_t)pName->size[depth]
				                        : executionSize;
				writeStaticBytes(atype, addr, executionBytes, storedSize);
			}
			else
			{
				Name *storage = findStackStorage(addr);
				if (storage == NULL)
				{
					error("initializer", "local string has no storage object");
				}
				for (n = 0; n < pName->size[depth]; ++n)
				{
					VALUE destination;
					VALUE character;
					setValue(ADDR, 0, ID.T_UCHAR, &destination);
					semanticIrLocalAddress(
					    &destination, storage->irLocal, addr - storage->address + n);
					setValue(VAL, 0, ID.T_UCHAR, &character);
					character.ival = (size_t)n < executionSize ? executionBytes[n] : 0;
					character.fConst = TRUE;
					semanticIrConstantInteger(&character);
					semanticIrBinary('=', &destination, &character);
				}
			}
			free(executionBytes);
			if (atype == AD_DATA)
			{
				ix.ixData += outString(ix.ixData, toString(ival)) + 1;
			}
			else
			{
				int size = pName->size[depth] / pName->size[depth + 1];
				int length = decodedStringLength(toString(ival)) + 1;
				if (length - 1 > size)
				{
					error("init'r", "initializer-string too long");
				}
				char *buf = xalloc(length);
				decodeString(buf, toString(ival));
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
	if (atype == AD_STACK)
	{
		clear(addr, addr + pName->size[depth]);
	}
	if (!fArray && getStruct(pName->dataType) == NULL)
	{
		initMember(pName, atype, depth, addr);
		(void)ispp(',');
		skip('}');
		return;
	}
	if (fArray)
	{
		int size1 = pName->size[depth + 1];
		int num = pName->size[depth] / size1;
		for (n = 0; !is('}');)
		{
			if (ispp('['))
			{
				n = constIntExpression();
				skip(']');
				skip('=');
			}
			if (n < 0)
			{
				error("initializer", "negative array designator");
			}
			if (n >= num)
			{
				error("init'r", "index too large");
			}
			initializer(pName, atype, depth + 1, addr + size1 * n++);
			if (!ispp(','))
			{
				break;
			}
		}
	}
	else
	{
		Name *aggregate = getStruct(pName->dataType);
		for (n = 0; !is('}');)
		{
			Name *pChild;
			if (ispp('.'))
			{
				int memberId = cd.token[ix.tix++].ival;
				n = memberIndex(pName->dataType, memberId, &pChild);
				skip('=');
			}
			else
			{
				pChild = getMember(pName->dataType, n);
			}
			if (pChild == NULL)
			{
				error("init'r", "too many field init");
			}
			++n;
			initializer(pChild, atype, 0, addr + pChild->address);
			if (!ispp(','))
			{
				break;
			}
			if (aggregate != NULL && (aggregate->type & NM_UNION) != 0 && !is('}'))
			{
				error("init'r", "only one union member may be initialized");
			}
		}
	}
	skip('}');
}

/*============================================================================
 * Variable Declaration
 *============================================================================*/

static void variableDeclaration(Name *pStruct, int status)
{
	Name *pName;
	int fStatic = ispp(ID.STATIC);
	int fExtern = ispp(ID.EXTERN);
	int fImp = is(ID.DECLSP);
	if (fStatic && fExtern)
	{
		error("declaration", "static and extern cannot be combined");
	}

	typeSpecifier();
	do
	{
		varDeclarator(FALSE);
		if (status == ST_FUNC)
		{
			if (!fStatic)
			{
				cd.baseSpace = -align(-cd.baseSpace, var.alignment);
				cd.baseSpace -= var.width * var.length;
				pName = newName(NM_VAR, var.type, var.id, AD_STACK, cd.baseSpace);
			}
			else
			{
				ix.ixData = align(ix.ixData, var.alignment);
				pName = newName(NM_VAR, var.type, var.id, AD_DATA, ix.ixData);
			}
			putName(cd.currTable, pName);
		}
		else if ((fImp || (fExtern && !is('='))) && status == ST_GVAR)
		{
			pName = getNameFromTable(globTable, NM_VAR, var.id);
			if (pName == NULL)
			{
				pName = appendName(globTable, NM_VAR, var.type, var.id, AD_IMPORT, 0);
			}
			else if (pName->dataType != var.type)
			{
				error("declaration", "incompatible redeclaration of '%s'", toString(var.id));
			}
		}
		else if (status == ST_GVAR)
		{
			ix.ixData = align(ix.ixData, var.alignment);
			pName = getNameFromTable(globTable, NM_VAR, var.id);
			if (pName != NULL && pName->addrType != AD_IMPORT)
			{
				error("declaration", "redefinition of '%s'", toString(var.id));
			}
			if (pName == NULL)
			{
				pName = appendName(globTable,
				                   NM_VAR | (fStatic ? NM_STATIC : 0),
				                   var.type,
				                   var.id,
				                   AD_DATA,
				                   ix.ixData);
			}
			else
			{
				pName->type = NM_VAR | (fStatic ? NM_STATIC : 0);
				pName->dataType = var.type;
				pName->addrType = AD_DATA;
				pName->address = ix.ixData;
			}
		}
		else
		{
			int isUnion = (pStruct->type & NM_UNION) != 0;
			int limit = cmd.target->dataLayout.subsequentMemberAlignmentLimit;
			if (!isUnion && pStruct->pBgn != NULL && limit != 0 && var.alignment > limit)
			{
				var.alignment = limit;
			}
			if (!isUnion)
			{
				cd.offset = align(cd.offset, var.alignment);
			}
			pName = xalloc(sizeof(Name));
			pName->type = NM_ATTR;
			pName->idName = var.id;
			pName->dataType = var.type;
			pName->address = isUnion ? 0 : cd.offset;
			if (pStruct->pBgn == NULL)
			{
				pStruct->pBgn = pName;
			}
			else
			{
				pStruct->pEnd->pNext = pName;
			}
			pStruct->pEnd = pName;
			if (isUnion)
			{
				int memberSize = var.width * var.length;
				if (memberSize > cd.offset)
				{
					cd.offset = memberSize;
				}
			}
			else
			{
				cd.offset += var.width * var.length;
			}
		}
		setAttr(pName);
		if (status == ST_GVAR)
		{
			IrGlobal *global = irFindGlobal(&compiler.ir, pName->irSymbol);
			IrType storageType;
			int totalPointers = pName->ptrs + getPtr(pName->dataType);
			Name *aggregate = getStruct(canonicalType(pName->dataType));

			if (pName->arrays > 0 || (aggregate != NULL && totalPointers == 0))
			{
				storageType = irTypeInteger(CHAR_BIT, 1, 1U);
			}
			else
			{
				storageType = irTypeForCObject(pName->dataType, totalPointers);
			}
			if (global == NULL)
			{
				global = irAddGlobal(&compiler.ir,
				                     pName->irSymbol,
				                     storageType,
				                     (size_t)pName->size[0],
				                     var.alignment,
				                     pName->addrType == AD_IMPORT,
				                     0);
			}
			else if (pName->addrType != AD_IMPORT)
			{
				global->type = storageType;
				global->zeroFillSize = (size_t)pName->size[0];
				global->alignment = var.alignment;
				global->isExternal = FALSE;
			}
			global->isInternal = fStatic;
		}
		if (status == ST_FUNC && semanticIrActive())
		{
			IrType storageType;
			int totalPointers = pName->ptrs + getPtr(pName->dataType);
			Name *aggregate = getStruct(canonicalType(pName->dataType));
			if (pName->arrays > 0 || (aggregate != NULL && totalPointers == 0))
			{
				storageType = irTypeInteger(CHAR_BIT, 1, 1U);
			}
			else
			{
				storageType = irTypeForCObject(pName->dataType, totalPointers);
			}
			if (fStatic)
			{
				IrGlobal *global;

				pName->irSymbol = irCreateAnonymousSymbol(&compiler.ir);
				global = irAddGlobal(&compiler.ir,
				                     pName->irSymbol,
				                     storageType,
				                     (size_t)pName->size[0],
				                     var.alignment,
				                     0,
				                     0);
				global->isInternal = TRUE;
			}
			else
			{
				pName->irLocal = irAddLocal(compiler.irBuilder.function,
				                            pName->idName,
				                            storageType,
				                            (size_t)pName->size[0],
				                            var.alignment,
				                            -1);
			}
		}
		if (ispp('='))
		{
			int ixString = ix.ixData + pName->size[0];
			IrGlobal *global = irFindGlobal(&compiler.ir, pName->irSymbol);

			if (global != NULL)
			{
				unsigned char *zeroes = xalloc(global->zeroFillSize);

				memset(zeroes, 0, global->zeroFillSize);
				irSetGlobalInitializer(global, zeroes, global->zeroFillSize);
				free(zeroes);
			}
			if (status == ST_GVAR)
			{
				ix.ixString = ixString;
			}
			initializer(pName, pName->addrType, 0, pName->address);
			if (ix.ixString > ixString)
			{
				ix.ixData = ix.ixString;
			}
			if (status == ST_GVAR && ix.ixData < pName->address + pName->size[0])
			{
				ix.ixData = pName->address + pName->size[0];
			}
		}
		else if (!fImp && !fExtern && (status == ST_GVAR || fStatic))
		{
			ix.ixZero = align(ix.ixZero, var.alignment);
			pName->addrType = AD_ZERO;
			pName->address = ix.ixZero;
			ix.ixZero += pName->size[0];
		}
	} while (ispp(','));
}

/*============================================================================
 * Struct Declaration
 *============================================================================*/

static void structDeclaration(void)
{
	int isTypedef = ispp(ID.TYPEDEF);
	int isUnion;
	Name *alias = NULL;

	isUnion = ispp(ID.UNION);
	if (!isUnion)
	{
		skip(ID.STRUCT);
	}
	int tagId = cd.token[ix.tix++].ival;
	Name *pName = newName(NM_STRUCT | (isUnion ? NM_UNION : 0), tagId, tagId, -1, 0);
	cd.offset = 0;
	int sizeAlign = 1;
	for (skip('{'); !ispp('}'); skip(';'))
	{
		variableDeclaration(pName, ST_STRUCT);
		int memberAlignment = var.alignment;
		if (memberAlignment > sizeAlign)
		{
			sizeAlign = memberAlignment;
		}
	}
	if (sizeAlign > 1)
	{
		cd.offset = align(cd.offset, sizeAlign);
	}
	pName->size[0] = cd.offset;
	pName->alignment = sizeAlign;
	putName(globTable, pName);
	if (!isTypedef)
	{
		if (!is(';'))
		{
			error("declaration", "declarator after aggregate definition is not supported");
		}
		skip(';');
		return;
	}
	pName->ptrs = ispp('*');
	int nameId = cd.token[ix.tix++].ival;
	alias = nameId == tagId
	            ? pName
	            : newName(NM_STRUCT | NM_TYPEDEF | (isUnion ? NM_UNION : 0), nameId, tagId, -1, 0);
	alias->ptrs = pName->ptrs;
	alias->size[0] = pName->size[0];
	alias->alignment = pName->alignment;
	alias->pBgn = pName->pBgn;
	alias->pEnd = pName->pEnd;
	putName(globTable, alias);
	cd.hash.tbl[nameId].val = (void *)((intptr_t)cd.hash.tbl[nameId].val | AT_TYPE);
	skip(';');
}

/*============================================================================
 * Enum Definition
 *============================================================================*/

static void enumDefinition(void)
{
	int val = 0;
	int isTypedef = ispp(ID.TYPEDEF);
	int tagId = -1;
	skip(ID.ENUM);
	if (!is('{'))
	{
		tagId = cd.token[ix.tix++].ival;
	}
	for (skip('{'); !ispp('}'); ispp(','))
	{
		int hid = cd.token[ix.tix++].ival;
		Name *pName = appendName(globTable, NM_ENUM, 0, hid, -1, val++);
		if (ispp('='))
		{
			val = constIntExpression();
			pName->address = val++;
		}
	}
	if (tagId >= 0)
	{
		(void)defineTypeAlias(tagId, ID.T_INT, 0, 4);
	}
	if (isTypedef)
	{
		int aliasId = cd.token[ix.tix++].ival;
		if (aliasId != tagId)
		{
			(void)defineTypeAlias(aliasId, tagId >= 0 ? tagId : ID.T_INT, 0, 4);
		}
	}
	skip(';');
}

/*============================================================================
 * Parameter Declaration
 *============================================================================*/

static Name *parameterDeclaration(void)
{
	Name *pName = NULL;
	typeSpecifier();
	varDeclarator(TRUE);
	if (ispp('(') && !ispp(')'))
	{
		error("paramDecl", "')' expected");
	}
	if (var.id >= 0)
	{
		pName = appendName(cd.currTable, NM_VAR, var.type, var.id, AD_STACK, cd.baseSpace);
		setAttr(pName);
	}
	cd.baseSpace += var.width;
	return pName;
}

static int irFunctionIsDefined(IrSymbolId symbol)
{
	int functionIndex;

	for (functionIndex = 0; functionIndex < compiler.ir.functionCount; ++functionIndex)
	{
		if (compiler.ir.functions[functionIndex].symbol == symbol)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*============================================================================
 * Function Definition
 *============================================================================*/

static void functionDefinition(void)
{
	int callConv = NM_CDECL;
	int fStatic = ispp(ID.STATIC);
	(void)ispp(ID.EXTERN);
	int fRtnStmt = FALSE;
	int fProtoType = FALSE;
	int ixCodeSave = ix.ixCode;
	int ixSp, argc, unnamedParameters = 0, ptrs = 0;
	PTRS_TYPE argpt[256];
	Name *parameterNames[256];
	IrType parameterTypes[256];
	int fixedParameterCount;
	memset(parameterNames, 0, sizeof(parameterNames));
	int fExp = is(ID.DECLSP);
	typeSpecifier();
	if (varDeclarator(FALSE) == NM_WINAPI)
	{
		callConv = NM_WINAPI;
	}
	if (var.id < 0)
	{
		error("funcDef", "function name expected before '%s'", toStr(ix.tix));
	}
	int idFunc = var.id;
	createNameTable(var.id);
	outCode2((fExp ? exp_ : fn_), var.id);
	Name *pName = getNameFromTable(globTable, NM_CDECL, var.id);
	if (pName != NULL && fStatic && irFunctionIsDefined(pName->irSymbol))
	{
		char key[128];

		formatNameKey(key, sizeof(key), NM_CDECL, toString(var.id));
		(void)hashRemove(key, &cd.block[globTable].hash);
		free(pName->argpt);
		free(pName);
		pName = NULL;
	}
	if (pName == NULL)
	{
		pName = appendName(
		    globTable, callConv | (fStatic ? NM_STATIC : 0), var.type, var.id, AD_CODE, 0);
		pName->ptrs = var.pointers + var.arrays + getPtr(var.type);
	}
	else if (pName->dataType != var.type || (pName->type & (NM_CDECL | NM_WINAPI | NM_STATIC)) !=
	                                            (callConv | (fStatic ? NM_STATIC : 0)))
	{
		error("funcDef", "incompatible types for redefinition of '%s'", toString(var.id));
	}
	if (fStatic && pName->irSymbol >= 0)
	{
		pName->irSymbol = irCreateAnonymousSymbol(&compiler.ir);
	}
	cd.baseSpace = 4 * 2;
	outCode1(xent);
	ixSp = ix.ixCode;
	outCode2(sub_esp, 0);
	argc = 0;
	skip('(');
	if (is(ID.T_VOID) && isN(')', 1))
	{
		ix.tix += 2;
	}
	else
	{
		for (; !ispp(')'); ispp(','))
		{
			int fD3 = ispp(ID.DOTS3);
			Name *parameterName = NULL;
			if (!fD3)
			{
				parameterName = parameterDeclaration();
				if (var.id < 0)
				{
					++unnamedParameters;
				}
				ptrs = var.pointers + var.arrays + getPtr(var.type);
			}
			if (argc >= (int)(sizeof(argpt) / sizeof(argpt[0])))
			{
				error("funcDef", "too many parameters in '%s'", toString(idFunc));
			}
			argpt[argc].ptrs = fD3 ? 0 : ptrs;
			argpt[argc++].type = fD3 ? ID.DOTS3 : var.type;
			parameterNames[argc - 1] = parameterName;
		}
	}
	pName->argc = argc;
	fixedParameterCount = argc > 0 && argpt[argc - 1].type == ID.DOTS3 ? argc - 1 : argc;
	int sizeRet = callConv == NM_WINAPI ? (cd.baseSpace - 8) : 0;
	free(pName->argpt);
	pName->argpt = NULL;
	if (argc > 0)
	{
		pName->argpt = calloc(argc, sizeof(PTRS_TYPE));
		memcpy(pName->argpt, argpt, argc * sizeof(PTRS_TYPE));
	}
	cd.baseSpace = 0;
	if (ispp(';'))
	{
		fProtoType = TRUE;
	}
	else
	{
		IrFunction *irFunction;
		StatementContext context = {0, 0, sizeRet, IR_BLOCK_NONE, IR_BLOCK_NONE};
		int parameterIndex;
		if (unnamedParameters != 0)
		{
			error("funcDef", "parameter names are required in a function definition");
		}
		for (parameterIndex = 0; parameterIndex < fixedParameterCount; ++parameterIndex)
		{
			parameterTypes[parameterIndex] =
			    irTypeForCObject(argpt[parameterIndex].type, argpt[parameterIndex].ptrs);
		}
		irFunction = irBuilderBeginFunction(&compiler.irBuilder,
		                                    pName->irSymbol,
		                                    irTypeForCObject(pName->dataType, pName->ptrs),
		                                    parameterTypes,
		                                    fixedParameterCount);
		irFunction->callingConvention = callConv == NM_WINAPI ? IR_CALL_WINAPI : IR_CALL_C;
		irFunction->isVariadic = fixedParameterCount != argc;
		irFunction->isInternal = fStatic;
		irFunction->isExported = fExp;
		for (parameterIndex = 0; parameterIndex < fixedParameterCount; ++parameterIndex)
		{
			Name *parameter = parameterNames[parameterIndex];
			IrValueId parameterValue;
			IrValueId address;
			IrType pointerType;
			if (parameter == NULL)
			{
				continue;
			}
			parameter->irLocal =
			    irAddLocal(irFunction,
			               parameter->idName,
			               parameterTypes[parameterIndex],
			               (size_t)sizeOfObjectType(parameter->dataType, parameter->ptrs),
			               alignmentOfObjectType(parameter->dataType, parameter->ptrs),
			               parameterIndex);
			parameterValue = irBuilderEmitParameter(
			    &compiler.irBuilder, parameterTypes[parameterIndex], parameterIndex);
			pointerType = irTypePointer((unsigned int)cmd.target->dataLayout.pointerSize * CHAR_BIT,
			                            cmd.target->dataLayout.pointerAlignment);
			address =
			    irBuilderEmitLocalAddress(&compiler.irBuilder, pointerType, parameter->irLocal, 0);
			irBuilderEmitStore(
			    &compiler.irBuilder, parameterTypes[parameterIndex], address, parameterValue);
		}
		fRtnStmt = compoundStatement(&context);
		if (!irBuilderBlockTerminated(&compiler.irBuilder))
		{
			IrType returnType = irFunction->returnType;
			IrValueId returnValue = IR_VALUE_NONE;
			if (returnType.kind != IR_TYPE_VOID)
			{
				returnValue = strcmp(toString(idFunc), "main") == 0
				                  ? irBuilderEmitInteger(&compiler.irBuilder, returnType, 0U)
				                  : irBuilderEmitUndefined(&compiler.irBuilder, returnType);
			}
			irBuilderEmitReturn(&compiler.irBuilder, returnValue);
		}
		irBuilderEndFunction(&compiler.irBuilder);
	}
	if (cd.baseSpace < 0)
	{
		cd.baseSpace = -align(-cd.baseSpace, 4);
	}
	if (cd.baseSpace != 0)
	{
		cd.pCode[ixSp].num = -cd.baseSpace;
	}
	else
	{
		memmove(&cd.pCode[ixSp], &cd.pCode[ixSp + 1], (--ix.ixCode - ixSp) * sizeof(INSTRUCT));
	}
	if (!fRtnStmt)
	{
		outCode2(xret, sizeRet);
	}
	deleteNameTable();
	if (fProtoType)
	{
		ix.ixCode = ixCodeSave;
	}
	else
	{
		intptr_t attributes = (intptr_t)cd.hash.tbl[idFunc].val;
		attributes |= fExp ? AT_EXPT : AT_USER;
		cd.hash.tbl[idFunc].val = (void *)attributes;
	}
}

/*============================================================================
 * Statements
 *============================================================================*/

static void ifStatement(const StatementContext *context)
{
	int locElse = loc(), locEnd = -1;
	VALUE condition;
	IrBlockId thenBlock;
	IrBlockId elseBlock;
	IrBlockId mergeBlock;
	ix.tix++;
	skip('(');
	expression(VAL, &condition);
	skip(')');
	thenBlock = semanticIrCreateBlock();
	elseBlock = semanticIrCreateBlock();
	mergeBlock = semanticIrCreateBlock();
	semanticIrBranchCondition(&condition, thenBlock, elseBlock);
	semanticIrSelectBlock(thenBlock);
	jumpFalse(locElse);
	int fReturn = is(ID.RETURN);
	statement(context);
	semanticIrBranch(mergeBlock);
	if (is(ID.ELSE) && !fReturn)
	{
		outCode2(jmp, locEnd = loc());
	}
	outCode3(loc_, locElse, 'E');
	if (ispp(ID.ELSE))
	{
		semanticIrSelectBlock(elseBlock);
		statement(context);
		semanticIrBranch(mergeBlock);
		if (locEnd >= 0)
		{
			outCode2(loc_, locEnd);
		}
	}
	else
	{
		semanticIrSelectBlock(elseBlock);
		semanticIrBranch(mergeBlock);
	}
	semanticIrSelectBlock(mergeBlock);
}

static void forStatement(int isFOR, const StatementContext *outerContext)
{
	int locExpr2 = loc(), locExpr3 = 0, locStmt = loc(), locNext = loc();
	int declarationScope = FALSE;
	VALUE condition;
	IrBlockId conditionBlock;
	IrBlockId incrementBlock;
	IrBlockId bodyBlock;
	IrBlockId exitBlock;
	StatementContext loopContext;
	ix.tix++;
	skip('(');
	if (isFOR)
	{
		if (!is(';'))
		{
			if (isTypeSpecifier(ix.tix))
			{
				createNameTable(-1);
				declarationScope = TRUE;
				variableDeclaration(NULL, ST_FUNC);
			}
			else
			{
				VALUE initializerValue;
				expression(VAL, &initializerValue);
			}
		}
		skip(';');
	}
	conditionBlock = semanticIrCreateBlock();
	incrementBlock = semanticIrCreateBlock();
	bodyBlock = semanticIrCreateBlock();
	exitBlock = semanticIrCreateBlock();
	semanticIrBranch(conditionBlock);
	semanticIrSelectBlock(conditionBlock);
	outCode2(loc_, locExpr2);
	int bgn = ix.ixCode;
	if (!is(';'))
	{
		expression(VAL, &condition);
	}
	else
	{
		outCode2(mov_eax, 1);
		setValue(VAL, 0, ID.T_INT, &condition);
		condition.ival = 1;
		condition.fConst = TRUE;
		semanticIrConstantInteger(&condition);
	}
	if (isFOR)
	{
		skip(';');
	}
	semanticIrBranchCondition(&condition, bodyBlock, exitBlock);
	jumpFalse(locNext);
	outCode2(jmp, locStmt);
	if (cd.pCode[bgn].inst == mov_eax && cd.pCode[bgn + 1].inst == test_eax_eax)
	{
		ix.ixCode = bgn;
		outCode2(jmp, cd.pCode[bgn].num == 0 ? locNext : locStmt);
	}
	int bgnExpr = ix.ixCode;
	outCode2(loc_, locExpr3 = loc());
	semanticIrSelectBlock(incrementBlock);
	if (isFOR && !is(')'))
	{
		VALUE incrementValue;
		expression(VAL, &incrementValue);
	}
	semanticIrBranch(conditionBlock);
	skip(')');
	int bgnStmt = ix.ixCode;
	outCode3(loc_, locStmt, '1');
	semanticIrSelectBlock(bodyBlock);
	memcpy(&loopContext, outerContext, sizeof(loopContext));
	loopContext.breakLocation = locNext;
	loopContext.continueLocation = locExpr3;
	loopContext.breakBlock = exitBlock;
	loopContext.continueBlock = incrementBlock;
	statement(&loopContext);
	semanticIrBranch(incrementBlock);
	int endStmt = ix.ixCode;
	int lenStmt = endStmt - bgnStmt;
	int lenBoth = endStmt - bgnExpr;
	if (bgnStmt - bgnExpr > 0 && lenStmt > 0)
	{
		reallocCode(lenStmt);
		memmove(&cd.pCode[bgnExpr + lenStmt], &cd.pCode[bgnExpr], lenBoth * sizeof(INSTRUCT));
		memmove(&cd.pCode[bgnExpr], &cd.pCode[bgnStmt + lenStmt], lenStmt * sizeof(INSTRUCT));
	}
	outCode2(jmp, locExpr2);
	outCode3(loc_, locNext, '0');
	semanticIrSelectBlock(exitBlock);
	if (declarationScope)
	{
		deleteNameTable();
	}
}

static void whileStatement(const StatementContext *context)
{
	forStatement(FALSE, context);
}

static void doStatement(const StatementContext *outerContext)
{
	int locStmt = loc(), locCondition = loc(), locBreak = loc();
	IrBlockId bodyBlock = semanticIrCreateBlock();
	IrBlockId conditionBlock = semanticIrCreateBlock();
	IrBlockId exitBlock = semanticIrCreateBlock();
	StatementContext loopContext;
	VALUE condition;
	ix.tix++;
	memcpy(&loopContext, outerContext, sizeof(loopContext));
	semanticIrBranch(bodyBlock);
	semanticIrSelectBlock(bodyBlock);
	outCode2(loc_, locStmt);
	loopContext.breakLocation = locBreak;
	loopContext.continueLocation = locCondition;
	loopContext.breakBlock = exitBlock;
	loopContext.continueBlock = conditionBlock;
	compoundStatement(&loopContext);
	semanticIrBranch(conditionBlock);
	if (!ispp(ID.WHILE))
	{
		error("doStmt", "while expected");
	}
	outCode2(loc_, locCondition);
	semanticIrSelectBlock(conditionBlock);
	skip('(');
	expression(VAL, &condition);
	skip(')');
	skip(';');
	semanticIrBranchCondition(&condition, bodyBlock, exitBlock);
	jumpTrue(locStmt);
	outCode2(loc_, locBreak);
	semanticIrSelectBlock(exitBlock);
}

static void returnStatement(const StatementContext *context)
{
	VALUE value;
	VALUE *returnValue = NULL;
	ix.tix++;
	if (!is(';'))
	{
		expression(VAL, &value);
		returnValue = &value;
	}
	skip(';');
	semanticIrReturn(returnValue);
	outCode2(xret, context->returnSize);
}

static void continueStatement(const StatementContext *context)
{
	if (context->continueLocation <= 0)
	{
		error("", "continue statement not within a loop");
	}
	ix.tix++;
	skip(';');
	semanticIrBranch(context->continueBlock);
	outCode2(jmp, context->continueLocation);
}

static void breakStatement(const StatementContext *context)
{
	if (context->breakLocation <= 0)
	{
		error("", "break statement not within loop or switch");
	}
	ix.tix++;
	skip(';');
	semanticIrBranch(context->breakBlock);
	outCode2(jmp, context->breakLocation);
}

static void switchStatement(const StatementContext *outerContext)
{
	int locCase = loc(), locStmt = loc(), locBreak;
	int fDefault = 0;
	VALUE switchValue;
	IrLocalId switchLocal;
	IrBlockId testBlock;
	IrBlockId exitBlock;
	IrBlockId fallthroughBlock = IR_BLOCK_NONE;
	StatementContext switchContext;
	memcpy(&switchContext, outerContext, sizeof(switchContext));
	ix.tix++;
	skip('(');
	expression(VAL, &switchValue);
	skip(')');
	switchLocal = semanticIrCreateTemporary(switchValue.type, switchValue.ptrs);
	semanticIrStoreTemporary(switchLocal, switchValue.type, switchValue.ptrs, &switchValue);
	testBlock = semanticIrActive() ? compiler.irBuilder.block : IR_BLOCK_NONE;
	exitBlock = semanticIrCreateBlock();
	skip('{');
	locBreak = loc();
	switchContext.breakLocation = locBreak;
	switchContext.breakBlock = exitBlock;
	while (!ispp('}'))
	{
		if (ispp(ID.CASE))
		{
			int caseValue;
			IrBlockId bodyBlock = semanticIrCreateBlock();
			IrBlockId nextTestBlock = semanticIrCreateBlock();
			VALUE testedValue;
			VALUE constantValue;
			if (fallthroughBlock != IR_BLOCK_NONE)
			{
				semanticIrSelectBlock(fallthroughBlock);
				semanticIrBranch(bodyBlock);
			}
			semanticIrSelectBlock(testBlock);
			outCode2(loc_, locCase);
			caseValue = constIntExpression();
			outCode2(cmp_eax, caseValue);
			setValue(VAL, switchValue.ptrs, switchValue.type, &testedValue);
			semanticIrLoadTemporary(switchLocal, switchValue.type, switchValue.ptrs, &testedValue);
			setValue(VAL, 0, ID.T_INT, &constantValue);
			constantValue.ival = caseValue;
			constantValue.fConst = TRUE;
			semanticIrConstantInteger(&constantValue);
			semanticIrBinary(id2("=="), &testedValue, &constantValue);
			semanticIrBranchCondition(&testedValue, bodyBlock, nextTestBlock);
			skip(':');
			outCode2(jnz, locCase = loc());
			outCode2(loc_, locStmt);
			semanticIrSelectBlock(bodyBlock);
			if (!is(ID.CASE))
			{
				do
				{
					statement(&switchContext);
				} while (!is(ID.CASE) && !is(ID.DEFAULT) && !is('}'));
			}
			fallthroughBlock = semanticIrActive() ? compiler.irBuilder.block : IR_BLOCK_NONE;
			testBlock = nextTestBlock;
			outCode2(jmp, locStmt = loc());
		}
		else if ((fDefault = ispp(ID.DEFAULT)))
		{
			IrBlockId defaultBlock = semanticIrCreateBlock();
			if (fallthroughBlock != IR_BLOCK_NONE)
			{
				semanticIrSelectBlock(fallthroughBlock);
				semanticIrBranch(defaultBlock);
			}
			semanticIrSelectBlock(testBlock);
			semanticIrBranch(defaultBlock);
			outCode2(loc_, locCase);
			skip(':');
			outCode2(loc_, locStmt);
			semanticIrSelectBlock(defaultBlock);
			do
			{
				statement(&switchContext);
			} while (!is('}'));
			fallthroughBlock = semanticIrActive() ? compiler.irBuilder.block : IR_BLOCK_NONE;
		}
		else
		{
			error("switchStmt", "'case' or 'default' expected");
		}
	}
	if (!fDefault)
	{
		if (fallthroughBlock != IR_BLOCK_NONE)
		{
			semanticIrSelectBlock(fallthroughBlock);
			semanticIrBranch(exitBlock);
		}
		semanticIrSelectBlock(testBlock);
		semanticIrBranch(exitBlock);
		outCode2(loc_, locCase);
		outCode2(loc_, locStmt);
	}
	else if (fallthroughBlock != IR_BLOCK_NONE)
	{
		semanticIrSelectBlock(fallthroughBlock);
		semanticIrBranch(exitBlock);
	}
	outCode2(loc_, locBreak);
	semanticIrSelectBlock(exitBlock);
}

static void statement(const StatementContext *context)
{
	if (is(';'))
	{
		ix.tix++;
	}
	else if (is('{'))
	{
		compoundStatement(context);
	}
	else if (is(ID.IF))
	{
		ifStatement(context);
	}
	else if (is(ID.FOR))
	{
		forStatement(TRUE, context);
	}
	else if (is(ID.WHILE))
	{
		whileStatement(context);
	}
	else if (is(ID.DO))
	{
		doStatement(context);
	}
	else if (is(ID.RETURN))
	{
		returnStatement(context);
	}
	else if (is(ID.BREAK))
	{
		breakStatement(context);
	}
	else if (is(ID.CONTINUE))
	{
		continueStatement(context);
	}
	else if (is(ID.SWITCH))
	{
		switchStatement(context);
	}
	else
	{
		expr(VAL);
		skip(';');
	}
}

static int compoundStatement(const StatementContext *context)
{
	createNameTable(-1);
	for (skip('{'); !ispp('}');)
	{
		if (isTypeSpecifier(ix.tix))
		{
			variableDeclaration(NULL, ST_FUNC);
			skip(';');
		}
		else
		{
			statement(context);
		}
		if (!is('}') && semanticIrActive() && irBuilderBlockTerminated(&compiler.irBuilder))
		{
			IrBlockId unreachableBlock = semanticIrCreateBlock();
			semanticIrSelectBlock(unreachableBlock);
		}
	}
	deleteNameTable();
	int n = ix.ixCode;
	while (--n > 0 && (cd.pCode[n].inst == loc_ || cd.pCode[n].inst == fn_))
		;
	return cd.pCode[n].inst == xret;
}

/* Parser initialization */

typedef struct _Keyword
{
	int *id;
	char *name;
} Keyword;

static char twoCharacterOperators[] = OPERATOR2;

static void init(void)
{
	int n;
	char s[2], s2[3];
	Keyword dataType[] = {{&ID.T_VOID, "void"},
	                      {&ID.T_BOOL, "_Bool"},
	                      {&ID.T_CHAR, "char"},
	                      {&ID.T_SHORT, "short"},
	                      {&ID.T_INT, "int"},
	                      {&ID.T_FLOAT, "float"},
	                      {&ID.T_DOUBLE, "double"},
	                      {&ID.STATIC, "static"},
	                      {&ID.STRUCT, "struct"},
	                      {&ID.UNION, "union"}};
	Keyword keyword[] = {{&ID.IF, "if"},
	                     {&ID.ELSE, "else"},
	                     {&ID.WHILE, "while"},
	                     {&ID.DO, "do"},
	                     {&ID.FOR, "for"},
	                     {&ID.RETURN, "return"},
	                     {&ID.CONTINUE, "continue"},
	                     {&ID.BREAK, "break"},
	                     {&ID.SWITCH, "switch"},
	                     {&ID.CASE, "case"},
	                     {&ID.DEFAULT, "default"},
	                     {&ID.SIZEOF, "sizeof"},
	                     {&ID.DOTS3, "..."},
	                     {&ID.TYPEDEF, "typedef"},
	                     {&ID.HWINAPI, "WINAPI"},
	                     {&ID.ENUM, "enum"},
	                     {&ID.HCONST, "const"},
	                     {&ID.EXTERN, "extern"},
	                     {&ID.DECLSP, "__declspec"},
	                     {&ID.SIGNED, "signed"},
	                     {&ID.UNSIGNED, "unsigned"},
	                     {&ID.HLONG, "long"},
	                     {&ID.DLLIMPT, "dllimport"},
	                     {&ID.DLLEXPT, "dllexport"}};
	s[1] = '\0';
	for (n = ' ' + 1; n < 127; ++n)
	{
		s[0] = (char)n;
		if (isgraph(*s) && !isdigit(*s))
		{
			hashPut(s, NULL, &cd.hash);
		}
	}
	for (n = 0; twoCharacterOperators[n] != '\0'; n += 3)
	{
		s2[0] = twoCharacterOperators[n];
		s2[1] = twoCharacterOperators[n + 1];
		s2[2] = '\0';
		{
			int expected = id2(&twoCharacterOperators[n]);
			int actual = hashPut(s2, NULL, &cd.hash);
			if (expected != actual)
			{
				error("parser.init",
				      "operator '%s' hash mismatch: expected %d, got %d",
				      s2,
				      expected,
				      actual);
			}
		}
	}
	for (n = 0; (size_t)n < sizeof(dataType) / sizeof(Keyword); n++)
	{
		*(dataType[n].id) = hashPut(dataType[n].name, (void *)AT_TYPE, &cd.hash);
	}
	for (n = 0; (size_t)n < sizeof(keyword) / sizeof(Keyword); n++)
	{
		*(keyword[n].id) = hashPut(keyword[n].name, NULL, &cd.hash);
	}
	ID.T_SCHAR = hashPut("signed char", (void *)AT_TYPE, &cd.hash);
	ID.T_UCHAR = hashPut("unsigned char", (void *)AT_TYPE, &cd.hash);
	ID.T_USHORT = hashPut("unsigned short", (void *)AT_TYPE, &cd.hash);
	ID.T_UINT = hashPut("unsigned int", (void *)AT_TYPE, &cd.hash);
	ID.T_LONG = hashPut("long int", (void *)AT_TYPE, &cd.hash);
	ID.T_ULONG = hashPut("unsigned long int", (void *)AT_TYPE, &cd.hash);
}

/*============================================================================
 * Program
 *============================================================================*/

static int isFunctionDefinition(void)
{
	int cursor = ix.tix;
	int sawBuiltin = FALSE;
	if (cursor >= cd.nToken)
	{
		return FALSE;
	}
	if (cd.token[cursor].ival == ID.STATIC || cd.token[cursor].ival == ID.EXTERN)
	{
		++cursor;
	}
	if (cursor < cd.nToken && cd.token[cursor].ival == ID.DECLSP)
	{
		cursor += 4;
	}
	while (cursor < cd.nToken &&
	       (cd.token[cursor].ival == ID.HCONST || cd.token[cursor].ival == ID.SIGNED ||
	        cd.token[cursor].ival == ID.UNSIGNED || cd.token[cursor].ival == ID.HLONG ||
	        cd.token[cursor].ival == ID.T_BOOL || cd.token[cursor].ival == ID.T_CHAR ||
	        cd.token[cursor].ival == ID.T_SHORT || cd.token[cursor].ival == ID.T_INT ||
	        cd.token[cursor].ival == ID.T_FLOAT || cd.token[cursor].ival == ID.T_DOUBLE))
	{
		if (cd.token[cursor].ival != ID.HCONST)
		{
			sawBuiltin = TRUE;
		}
		++cursor;
	}
	if (!sawBuiltin)
	{
		if (cursor < cd.nToken &&
		    (cd.token[cursor].ival == ID.STRUCT || cd.token[cursor].ival == ID.UNION ||
		     cd.token[cursor].ival == ID.ENUM))
		{
			cursor += 2;
		}
		else
		{
			if (cursor >= cd.nToken || !isTypeSpecifier(cursor))
			{
				return FALSE;
			}
			++cursor;
		}
	}
	while (cursor < cd.nToken && cd.token[cursor].ival == '*')
	{
		++cursor;
	}
	if (cursor < cd.nToken && cd.token[cursor].ival == ID.HWINAPI)
	{
		++cursor;
	}
	return cursor + 1 < cd.nToken && cd.token[cursor + 1].ival == '(';
}

static void program(void)
{
	for (ix.tix = 0; ix.tix + 2 < cd.nToken;)
	{
		if (opt & oTRACE)
		{
			printToken(ix.tix);
		}
		if (((is(ID.STRUCT) || is(ID.UNION)) && isN('{', 1)) ||
		    (is(ID.TYPEDEF) && (isN(ID.STRUCT, 1) || isN(ID.UNION, 1)) && isN('{', 2)))
		{
			error("declaration", "anonymous struct and union types are not supported");
		}
		else if (((is(ID.STRUCT) || is(ID.UNION)) && ix.tix + 2 < cd.nToken &&
		          cd.token[ix.tix + 2].ival == '{') ||
		         (is(ID.TYPEDEF) && (isN(ID.STRUCT, 1) || isN(ID.UNION, 1)) &&
		          ix.tix + 3 < cd.nToken && cd.token[ix.tix + 3].ival == '{'))
		{
			structDeclaration();
		}
		else if ((is(ID.TYPEDEF) && isN(ID.ENUM, 1) && (isN('{', 2) || isN('{', 3))) ||
		         (is(ID.ENUM) && (isN('{', 1) || isN('{', 2))))
		{
			enumDefinition();
		}
		else if (is(ID.TYPEDEF))
		{
			typedefDeclaration();
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
		if (opt & oTOKEN)
		{
			printToken(n);
		}
	}
	cd.currTable = -1;
	createNameTable(id("<global>"));
	program();
	for (n = 0; n < ix.ixCode - 2; n++)
	{
		int inst = cd.pCode[n].inst;
		int num = cd.pCode[n].num;
		if ((inst == jge || inst == jl || inst == jz || inst == jnz) &&
		    cd.pCode[n + 1].inst == jmp && cd.pCode[n + 2].num == num)
		{
			cd.pCode[n].num = cd.pCode[n + 1].num;
			cd.pCode[n + 1].num = num;
			cd.pCode[n].inst = inst == jge ? jl : inst == jl ? jge : inst == jz ? jnz : jz;
		}
	}
	for (n = 0; n < ix.ixCode - 1; n++)
	{
		if (cd.pCode[n].inst == jmp && cd.pCode[n + 1].inst == loc_ &&
		    cd.pCode[n].num == cd.pCode[n + 1].num)
		{
			delCode(n);
		}
	}
	irVerifyModule(&compiler.ir);
}

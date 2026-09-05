/* Expression parsing, type analysis, constant folding, and IR emission. */

#include "cc.h"

static void logicalOrExpression(int mode, VALUE *value);
static void logicalAndExpression(int mode, VALUE *value);
static void orExpression(int mode, VALUE *value);
static void xorExpression(int mode, VALUE *value);
static void andExpression(int mode, VALUE *value);
static void equalityExpression(int mode, VALUE *value);
static void relationalExpression(int mode, VALUE *value);
static void shiftExpression(int mode, VALUE *value);
static void addExpression(int mode, VALUE *value);
static void mulExpression(int mode, VALUE *value);
static void castExpression(int mode, VALUE *value);
static void unaryExpression(int mode, VALUE *value);
static void primaryExpression(int mode, VALUE *value);

static double constantReal(const VALUE *value)
{
	if (isFloatingType(value->type))
	{
		return value->rval;
	}
	return isUnsignedType(value->type) ? (double)(uint32_t)value->ival : (double)value->ival;
}

/* Fold metadata separately from IR emission; never select machine instructions here. */
void infixOperation(int op, VALUE *left, VALUE *right, int leftBegin, int leftEnd)
{
	int comparison = op == id2("==") || op == id2("!=") || op == '<' || op == '>' ||
	                 op == id2("<=") || op == id2(">=");
	int shift = op == FRONTEND_OP_SHIFT_LEFT || op == FRONTEND_OP_SHIFT_RIGHT || op == id2("<<") ||
	            op == id2(">>");
	int resultType;
	int constant = left->fConst && right->fConst;
	int leftPointers = left->ptrs;
	int rightPointers = right->ptrs;
	int originalType = left->type;
	double realLeft = 0.0;
	double realRight = 0.0;
	uint32_t a = (uint32_t)left->ival;
	uint32_t b = (uint32_t)right->ival;
	int floating = isFloatingType(left->type) || isFloatingType(right->type);
	(void)leftBegin;
	(void)leftEnd;
	if (op == '=')
	{
		semanticIrBinary(op, left, right);
		left->fConst = FALSE;
		return;
	}
	if (floating && constant)
	{
		realLeft = constantReal(left);
		realRight = constantReal(right);
	}
	resultType = shift ? integerPromotion(left->type) : arithmeticType(left->type, right->type);
	semanticIrBinary(op, left, right);
	if (leftPointers > 0 || rightPointers > 0)
	{
		if (comparison || (op == '-' && leftPointers > 0 && rightPointers > 0))
		{
			left->type = ID.T_INT;
			left->ptrs = 0;
			left->constantSymbol = IR_SYMBOL_NONE;
		}
		else if (leftPointers > 0 && rightPointers == 0 && (op == '+' || op == '-'))
		{
			if (left->constantSymbol != IR_SYMBOL_NONE && right->fConst)
			{
				int delta = right->ival * sizeOfObjectType(originalType, leftPointers - 1);
				left->constantOffset += op == '+' ? delta : -delta;
				left->fConst = TRUE;
				return;
			}
		}
		else if (rightPointers > 0 && leftPointers == 0 && op == '+')
		{
			left->type = right->type;
			left->ptrs = rightPointers;
		}
		else
		{
			error("expression", "invalid pointer operands");
		}
		left->fConst = FALSE;
		return;
	}
	if (floating && (shift || op == '%' || op == '&' || op == '|' || op == '^'))
	{
		error("expression", "integer operands required");
	}
	if (constant && floating)
	{
		if (comparison)
		{
			a = op == id2("==")   ? realLeft == realRight
			    : op == id2("!=") ? realLeft != realRight
			    : op == '<'       ? realLeft < realRight
			    : op == '>'       ? realLeft > realRight
			    : op == id2("<=") ? realLeft <= realRight
			                      : realLeft >= realRight;
		}
		else
		{
			if (op == '+')
			{
				left->rval = realLeft + realRight;
			}
			else if (op == '-')
			{
				left->rval = realLeft - realRight;
			}
			else if (op == '*')
			{
				left->rval = realLeft * realRight;
			}
			else if (op == '/')
			{
				left->rval = realLeft / realRight;
			}
			else
			{
				error("expression", "invalid floating-point operator");
			}
			if (resultType == ID.T_FLOAT)
			{
				left->rval = (double)(float)left->rval;
			}
		}
	}
	else if (constant)
	{
		int isUnsigned = isUnsignedType(resultType);
		if (op == '+')
		{
			a += b;
		}
		else if (op == '-')
		{
			a -= b;
		}
		else if (op == '*')
		{
			a *= b;
		}
		else if (op == '/' || op == '%')
		{
			if (b == 0U || (!isUnsigned && (int32_t)a == INT32_MIN && (int32_t)b == -1))
			{
				error("expression", "invalid constant division");
			}
			if (isUnsigned)
			{
				a = op == '/' ? a / b : a % b;
			}
			else
			{
				a = (uint32_t)(op == '/' ? (int32_t)a / (int32_t)b : (int32_t)a % (int32_t)b);
			}
		}
		else if (shift)
		{
			if (b >= 32U)
			{
				error("expression", "shift count is outside the 32-bit target range");
			}
			if (op == FRONTEND_OP_SHIFT_LEFT || op == id2("<<"))
			{
				a <<= b;
			}
			else
			{
				a = isUnsigned ? a >> b : (uint32_t)((int32_t)a >> b);
			}
		}
		else if (op == '&')
		{
			a &= b;
		}
		else if (op == '|')
		{
			a |= b;
		}
		else if (op == '^')
		{
			a ^= b;
		}
		else if (op == id2("==") || op == id2("!="))
		{
			a = op == id2("==") ? a == b : a != b;
		}
		else if (comparison)
		{
			if (isUnsigned)
			{
				a = op == '<' ? a < b : op == '>' ? a > b : op == id2("<=") ? a <= b : a >= b;
			}
			else
			{
				int32_t sa = (int32_t)a;
				int32_t sb = (int32_t)b;
				a = op == '<'         ? sa < sb
				    : op == '>'       ? sa > sb
				    : op == id2("<=") ? sa <= sb
				                      : sa >= sb;
			}
		}
		else
		{
			error("expression", "invalid integer operator");
		}
	}
	left->ival = (int)a;
	left->type = comparison ? ID.T_INT : resultType;
	left->ptrs = 0;
	left->mode = VAL;
	left->fConst = constant;
	left->constantSymbol = IR_SYMBOL_NONE;
	left->object = NULL;
	left->arrayDepth = 0;
}

void expression(int mode, VALUE *pv)
{
	assignExpression(mode, pv);
	while (ispp(','))
	{
		assignExpression(mode, pv);
	}
}

/*============================================================================
 * Assign Expression
 *============================================================================*/

void assignExpression(int mode, VALUE *value)
{
	INDEX saved;
	VALUE right;
	const char *operatorText;
	int operation;
	int assignment;
	memcpy(&saved, &ix, sizeof(saved));
	irBuilderSuspend(&compiler.irBuilder);
	castExpression(ADDR, value);
	irBuilderResume(&compiler.irBuilder);
	operatorText = cd.token[ix.tix].token;
	assignment = strcmp(operatorText, "=") == 0 || strcmp(operatorText, "<<=") == 0 ||
	             strcmp(operatorText, ">>=") == 0 ||
	             (strchr("+-/*%|&^", operatorText[0]) != NULL && operatorText[1] == '=');
	memcpy(&ix, &saved, sizeof(ix));
	if (!assignment)
	{
		conditionalExpression(mode, value);
		return;
	}
	castExpression(ADDR, value);
	if (value->mode != ADDR)
	{
		error("assignment", "modifiable lvalue required");
	}
	operatorText = cd.token[ix.tix].token;
	operation = strcmp(operatorText, "<<=") == 0 || strcmp(operatorText, ">>=") == 0
	                ? id2(cd.token[ix.tix].token)
	                : cd.token[ix.tix].ival;
	++ix.tix;
	assignExpression(VAL, &right);
	semanticIrBinary(operation, value, &right);
	value->fConst = FALSE;
	value->constantSymbol = IR_SYMBOL_NONE;
	value->mode = VAL;
}

/*============================================================================
 * Conditional Expression
 *============================================================================*/

void conditionalExpression(int mode, VALUE *pv)
{
	int locFalse, locEnd;
	INDEX ixBgn;
	memcpy(&ixBgn, &ix, sizeof(ix));
	logicalOrExpression(mode, pv);
	if (ispp('?'))
	{
		VALUE condition;
		VALUE trueValue;
		VALUE falseValue;
		int resultType;
		int resultPointers;
		IrBlockId trueBlock = semanticIrCreateBlock();
		IrBlockId falseBlock = semanticIrCreateBlock();
		IrBlockId mergeBlock = semanticIrCreateBlock();
		IrBlockId trueEndBlock;
		IrBlockId falseEndBlock;
		IrLocalId resultLocal;
		memcpy(&condition, pv, sizeof(condition));
		semanticIrBranchCondition(&condition, trueBlock, falseBlock);
		semanticIrSelectBlock(trueBlock);
		outCode1(test_eax_eax);
		outCode2(jz, locFalse = loc());
		INSTRUCT *pC = &cd.pCode[ix.ixCode - 3];
		if (pC[0].inst == sete_eax && pC[1].inst == test_eax_eax && pC[2].inst == jz)
		{
			pC[2].inst = jnz;
			delCodes(ix.ixCode - 3, ix.ixCode - 1);
		}
		assignExpression(VAL, &trueValue);
		trueEndBlock = semanticIrActive() ? compiler.irBuilder.block : IR_BLOCK_NONE;
		outCode2(jmp, locEnd = loc());
		skip(':');
		outCode2(loc_, locFalse);
		semanticIrSelectBlock(falseBlock);
		assignExpression(VAL, &falseValue);
		falseEndBlock = semanticIrActive() ? compiler.irBuilder.block : IR_BLOCK_NONE;
		outCode2(loc_, locEnd);
		if (trueValue.ptrs > 0 || falseValue.ptrs > 0)
		{
			if (trueValue.ptrs == falseValue.ptrs &&
			    canonicalType(trueValue.type) == canonicalType(falseValue.type))
			{
				resultType = trueValue.type;
				resultPointers = trueValue.ptrs;
			}
			else if (trueValue.ptrs == 1 && falseValue.ptrs == 1 &&
			         (canonicalType(trueValue.type) == ID.T_VOID ||
			          canonicalType(falseValue.type) == ID.T_VOID))
			{
				resultType = ID.T_VOID;
				resultPointers = 1;
			}
			else if (trueValue.ptrs > 0 && falseValue.fConst && falseValue.ival == 0)
			{
				resultType = trueValue.type;
				resultPointers = trueValue.ptrs;
			}
			else if (falseValue.ptrs > 0 && trueValue.fConst && trueValue.ival == 0)
			{
				resultType = falseValue.type;
				resultPointers = falseValue.ptrs;
			}
			else
			{
				error("conditional expression", "incompatible pointer operands");
			}
		}
		else if (trueValue.type == ID.T_DOUBLE || falseValue.type == ID.T_DOUBLE)
		{
			resultType = ID.T_DOUBLE;
			resultPointers = 0;
		}
		else
		{
			resultType = usualIntegerType(trueValue.type, falseValue.type);
			resultPointers = 0;
		}
		resultLocal = semanticIrCreateTemporary(resultType, resultPointers);
		semanticIrSelectBlock(falseEndBlock);
		semanticIrStoreTemporary(resultLocal, resultType, resultPointers, &falseValue);
		semanticIrBranch(mergeBlock);
		semanticIrSelectBlock(trueEndBlock);
		semanticIrStoreTemporary(resultLocal, resultType, resultPointers, &trueValue);
		semanticIrBranch(mergeBlock);
		semanticIrSelectBlock(mergeBlock);
		setValue(VAL, resultPointers, resultType, pv);
		semanticIrLoadTemporary(resultLocal, resultType, resultPointers, pv);
		pv->fConst = FALSE;
	}
	if (pv->fConst)
	{
		int tixCurr = ix.tix;
		memcpy(&ix, &ixBgn, sizeof(ix));
		ix.tix = tixCurr;
		if (pv->ptrs > 0)
		{
			outCode3(mov_eax, pv->ival, AD_CONST);
		}
		else if (isIntegerType(pv->type))
		{
			outCode3(mov_eax, pv->ival, cd.pCode[ix.ixCode].attr);
		}
		else
		{
			loadAddr(AD_DATA, ix.ixData);
			outDataDouble(pv->rval);
			outCode1(fld_qax);
		}
	}
}

/*============================================================================
 * Logical Or Expression
 *============================================================================*/

static void logicalOrExpression(int mode, VALUE *pv)
{
	logicalAndExpression(mode, pv);
	if (!is2("||"))
	{
		return;
	}
	IrBlockId trueBlock = semanticIrCreateBlock();
	IrBlockId falseBlock = semanticIrCreateBlock();
	IrBlockId mergeBlock = semanticIrCreateBlock();
	IrLocalId resultLocal = semanticIrCreateTemporary(ID.T_INT, 0);
	int locTRUE = loc();
	int locNext = loc();
	while (is2pp("||"))
	{
		IrBlockId rightBlock = semanticIrCreateBlock();
		semanticIrBranchCondition(pv, trueBlock, rightBlock);
		semanticIrSelectBlock(rightBlock);
		jumpTrue(locTRUE);
		logicalAndExpression(mode, pv);
	}
	semanticIrBranchCondition(pv, trueBlock, falseBlock);
	jumpTrue(locTRUE);
	outCode2(mov_eax, 0);
	outCode2(jmp, locNext);
	outCode2(loc_, locTRUE);
	semanticIrSelectBlock(trueBlock);
	setValue(VAL, 0, ID.T_INT, pv);
	pv->ival = 1;
	pv->fConst = TRUE;
	semanticIrConstantInteger(pv);
	semanticIrStoreTemporary(resultLocal, ID.T_INT, 0, pv);
	semanticIrBranch(mergeBlock);
	outCode2(mov_eax, 1);
	semanticIrSelectBlock(falseBlock);
	setValue(VAL, 0, ID.T_INT, pv);
	pv->ival = 0;
	pv->fConst = TRUE;
	semanticIrConstantInteger(pv);
	semanticIrStoreTemporary(resultLocal, ID.T_INT, 0, pv);
	semanticIrBranch(mergeBlock);
	outCode2(loc_, locNext);
	semanticIrSelectBlock(mergeBlock);
	setValue(VAL, 0, ID.T_INT, pv);
	semanticIrLoadTemporary(resultLocal, ID.T_INT, 0, pv);
}

/*============================================================================
 * Logical And Expression
 *============================================================================*/

static void logicalAndExpression(int mode, VALUE *pv)
{
	orExpression(mode, pv);
	if (!is2("&&"))
	{
		return;
	}
	IrBlockId trueBlock = semanticIrCreateBlock();
	IrBlockId falseBlock = semanticIrCreateBlock();
	IrBlockId mergeBlock = semanticIrCreateBlock();
	IrLocalId resultLocal = semanticIrCreateTemporary(ID.T_INT, 0);
	int locFalse = loc();
	int locNext = loc();
	while (is2pp("&&"))
	{
		IrBlockId rightBlock = semanticIrCreateBlock();
		semanticIrBranchCondition(pv, rightBlock, falseBlock);
		semanticIrSelectBlock(rightBlock);
		jumpFalse(locFalse);
		orExpression(mode, pv);
	}
	semanticIrBranchCondition(pv, trueBlock, falseBlock);
	jumpFalse(locFalse);
	semanticIrSelectBlock(trueBlock);
	setValue(VAL, 0, ID.T_INT, pv);
	pv->ival = 1;
	pv->fConst = TRUE;
	semanticIrConstantInteger(pv);
	semanticIrStoreTemporary(resultLocal, ID.T_INT, 0, pv);
	semanticIrBranch(mergeBlock);
	outCode2(mov_eax, 1);
	outCode2(jmp, locNext);
	outCode3(loc_, locFalse, 'F');
	semanticIrSelectBlock(falseBlock);
	setValue(VAL, 0, ID.T_INT, pv);
	pv->ival = 0;
	pv->fConst = TRUE;
	semanticIrConstantInteger(pv);
	semanticIrStoreTemporary(resultLocal, ID.T_INT, 0, pv);
	semanticIrBranch(mergeBlock);
	outCode2(mov_eax, 0);
	outCode3(loc_, locNext, 'N');
	semanticIrSelectBlock(mergeBlock);
	setValue(VAL, 0, ID.T_INT, pv);
	semanticIrLoadTemporary(resultLocal, ID.T_INT, 0, pv);
}

/*============================================================================
 * Or Expression
 *============================================================================*/

static void orExpression(int mode, VALUE *pv)
{
	xorExpression(mode, pv);
	while (ispp('|'))
	{
		VALUE v2;
		outCode1(push_eax);
		xorExpression(mode, &v2);
		infixOperation('|', pv, &v2, -1, -1);
	}
}

/*============================================================================
 * Xor Expression
 *============================================================================*/

static void xorExpression(int mode, VALUE *pv)
{
	andExpression(mode, pv);
	while (ispp('^'))
	{
		VALUE v2;
		outCode1(push_eax);
		andExpression(mode, &v2);
		infixOperation('^', pv, &v2, -1, -1);
	}
}

/*============================================================================
 * And Expression
 *============================================================================*/

static void andExpression(int mode, VALUE *pv)
{
	equalityExpression(mode, pv);
	while (ispp('&'))
	{
		VALUE v2;
		int ixOp2 = ix.ixCode;
		outCode1(push_eax);
		equalityExpression(mode, &v2);
		infixOperation('&', pv, &v2, -1, ixOp2);
	}
}

/*============================================================================
 * Equality Expression
 *============================================================================*/

static void equalityExpression(int mode, VALUE *pv)
{
	int fEQ;
	relationalExpression(mode, pv);
	if ((fEQ = is2pp("==")) || is2pp("!="))
	{
		VALUE v2;
		int ixOp2 = ix.ixCode;
		if (pv->type != ID.T_DOUBLE)
		{
			outCode1(push_eax);
		}
		relationalExpression(mode, &v2);
		infixOperation(fEQ ? id2("==") : id2("!="), pv, &v2, -1, ixOp2);
	}
}

/*============================================================================
 * Relational Expression
 *============================================================================*/

static void relationalExpression(int mode, VALUE *pv)
{
	int fLT = 0, fGT = 0, fLE = 0;
	shiftExpression(mode, pv);
	if ((fLT = ispp('<')) || (fGT = ispp('>')) || (fLE = is2pp("<=")) || is2pp(">="))
	{
		VALUE v2;
		int ixOp2 = ix.ixCode;
		if (pv->type != ID.T_DOUBLE)
		{
			outCode1(push_eax);
		}
		shiftExpression(mode, &v2);
		infixOperation(fLT ? '<' : fGT ? '>' : fLE ? id2("<=") : id2(">="), pv, &v2, -1, ixOp2);
	}
}

/*============================================================================
 * Shift Expression
 *============================================================================*/

static void shiftExpression(int mode, VALUE *pv)
{
	int fShl;
	addExpression(mode, pv);
	if ((fShl = is2pp("<<")) || is2pp(">>"))
	{
		VALUE v2;
		outCode1(push_eax);
		addExpression(mode, &v2);
		infixOperation(fShl ? FRONTEND_OP_SHIFT_LEFT : FRONTEND_OP_SHIFT_RIGHT, pv, &v2, -1, -1);
	}
}

/*============================================================================
 * Add Expression
 *============================================================================*/

static void addExpression(int mode, VALUE *pv)
{
	int fAdd;
	mulExpression(mode, pv);
	while ((fAdd = ispp('+')) || ispp('-'))
	{
		VALUE v2;
		int ixOp2 = ix.ixCode;
		if (pv->type != ID.T_DOUBLE)
		{
			outCode1(push_eax);
		}
		mulExpression(mode, &v2);
		infixOperation(fAdd ? '+' : '-', pv, &v2, -1, ixOp2);
	}
}

/*============================================================================
 * Mul Expression
 *============================================================================*/

static void mulExpression(int mode, VALUE *pv)
{
	int fMul = 0, fDiv = 0;
	castExpression(mode, pv);
	while ((fMul = ispp('*')) || (fDiv = ispp('/')) || ispp('%'))
	{
		VALUE v2;
		int ixOp2 = ix.ixCode;
		if (pv->type != ID.T_DOUBLE)
		{
			outCode1(push_eax);
		}
		castExpression(mode, &v2);
		infixOperation(fMul ? '*' : fDiv ? '/' : '%', pv, &v2, -1, ixOp2);
	}
}

/*============================================================================
 * Cast Expression
 *============================================================================*/

static void castExpression(int mode, VALUE *value)
{
	int destination;
	int pointers;
	int source;
	double real = 0.0;
	if (!is('(') || !(isTypeSpecifier(ix.tix + 1) && !isN('(', 2)))
	{
		unaryExpression(mode, value);
		return;
	}
	++ix.tix;
	typeSpecifier();
	destination = canonicalType(var.type);
	pointers = getPtr(var.type);
	while (ispp('*'))
	{
		++pointers;
	}
	skip(')');
	castExpression(mode, value);
	source = canonicalType(value->type);
	if (value->fConst)
	{
		real = constantReal(value);
	}
	semanticIrCast(value, destination, pointers);
	if (value->fConst && pointers == 0 && destination != ID.T_VOID)
	{
		if (isFloatingType(destination))
		{
			value->rval = destination == ID.T_FLOAT ? (double)(float)real : real;
		}
		else if (destination == ID.T_BOOL)
		{
			value->ival = real != 0.0;
		}
		else if (isIntegerType(destination))
		{
			uint32_t bits =
			    isFloatingType(source)
			        ? (isUnsignedType(destination) ? (uint32_t)real : (uint32_t)(int32_t)real)
			        : (uint32_t)value->ival;
			int width = sizeOfDataType(destination);
			if (width == 1)
			{
				bits = isUnsignedType(destination) ? (uint32_t)(uint8_t)bits
				                                   : (uint32_t)(int32_t)(int8_t)bits;
			}
			else if (width == 2)
			{
				bits = isUnsignedType(destination) ? (uint32_t)(uint16_t)bits
				                                   : (uint32_t)(int32_t)(int16_t)bits;
			}
			value->ival = (int)bits;
		}
	}
	value->type = destination;
	value->ptrs = pointers;
	value->mode = VAL;
	value->objectSize = sizeOfObjectType(destination, pointers);
	value->object = NULL;
	value->arrayDepth = 0;
}

/*============================================================================
 * Unary Expression
 *============================================================================*/

static void unaryExpression(int mode, VALUE *pv)
{
	int fNEG = 0, fNOT = 0, fBITNOT = 0, fINC = 0;
	if (ispp('+') || (fNEG = ispp('-')) || (fNOT = ispp('!')) || (fBITNOT = ispp('~')))
	{
		int wasFloating;
		int wasUnsigned;
		double real;
		uint32_t integer;
		castExpression(VAL, pv);
		wasFloating = isFloatingType(pv->type);
		wasUnsigned = isUnsignedType(pv->type);
		real = pv->fConst ? constantReal(pv) : 0.0;
		integer = (uint32_t)pv->ival;
		if (fBITNOT && (pv->ptrs > 0 || !isIntegerType(pv->type)))
		{
			error("unary expression", "integer operand required for '~'");
		}
		if (!fNOT && pv->ptrs > 0)
		{
			error("unary expression", "arithmetic operand required");
		}
		semanticIrUnary(fNEG ? '-' : fNOT ? '!' : fBITNOT ? '~' : '+', pv);
		if (pv->fConst)
		{
			if (fNEG && wasFloating)
			{
				pv->rval = -real;
			}
			else if (fNEG)
			{
				if (!wasUnsigned && integer == 0x80000000U)
				{
					error("unary expression", "signed negation overflows");
				}
				pv->ival = (int)(0U - integer);
			}
			else if (fNOT)
			{
				pv->ival = real == 0.0;
			}
			else if (fBITNOT)
			{
				pv->ival = (int)~integer;
			}
		}
		if (fNOT)
		{
			pv->type = ID.T_INT;
			pv->ptrs = 0;
		}
		else if (isIntegerType(pv->type))
		{
			pv->type = integerPromotion(pv->type);
		}
		pv->mode = VAL;
	}
	else if ((fINC = is2pp("++")) || is2pp("--"))
	{
		unaryExpression(ADDR, pv);
		semanticIrIncrement(pv, fINC, FALSE);
		incdec(pv->type, pv->ptrs, fINC, 'a');
		loadValue(pv->type, pv->ptrs > 0);
	}
	else if (ispp('&'))
	{
		castExpression(ADDR, pv);
		pv->fAddr = TRUE;
		pv->ptrs++;
		pv->objectSize = 4;
		pv->irValue = pv->irAddress;
		pv->irAddress = IR_VALUE_NONE;
	}
	else if (ispp('*'))
	{
		castExpression(VAL, pv);
		pv->mode = ADDR;
		pv->ptrs--;
		pv->objectSize = sizeOfObjectType(pv->type, pv->ptrs);
		pv->irAddress = pv->irValue;
		pv->irValue = IR_VALUE_NONE;
		if (mode == VAL)
		{
			loadValue(pv->type, pv->ptrs > 0);
			semanticIrLoad(pv);
		}
	}
	else
	{
		primaryExpression(mode, pv);
	}
}

/*============================================================================
 * Function Call
 *============================================================================*/

static int checkarg(int pt1, int type1, int pt2, int type2, char *name, int nArg)
{
	char *f = "%s arg#%d assignment makes %s from %s";
	char *g = "%s arg#%d assignment from incompatible pointer type";
	if (type2 == ID.DOTS3)
	{
		return FALSE;
	}
	if (pt1 > 0 && pt2 == 0)
	{
		error("chk", f, name, nArg, "integer", "pointer");
	}
	if (pt1 == 0 && pt2 > 0)
	{
		error("chk", f, name, nArg, "pointer", "integer");
	}
	if (pt1 != pt2 && type1 != ID.T_VOID && type2 != ID.T_VOID)
	{
		error("chk", g, name, nArg);
	}
	return TRUE;
}

typedef struct _ParsedCall
{
	int *positions;
	IrValueId *arguments;
	int count;
	int stackBytes;
} ParsedCall;

static void parseCallArguments(const Name *signature, VALUE *value, ParsedCall *parsed)
{
	int capacity = 0;
	int fixedParameters = signature->argc;
	int variadic = FALSE;
	int checkTypes;
	int index;
	memset(parsed, 0, sizeof(*parsed));
	if (fixedParameters > 0 && signature->argpt[fixedParameters - 1].type == ID.DOTS3)
	{
		--fixedParameters;
		variadic = TRUE;
	}
	checkTypes = signature->argc >= 0 && signature->idName != id("main");
	skip('(');
	while (!ispp(')'))
	{
		int expectedType = ID.DOTS3;
		int expectedPointers = 0;
		if (parsed->count == capacity)
		{
			capacity = capacity == 0 ? 8 : capacity * 2;
			parsed->positions =
			    xrealloc(parsed->positions, (size_t)capacity * sizeof(*parsed->positions));
			parsed->arguments =
			    xrealloc(parsed->arguments, (size_t)capacity * sizeof(*parsed->arguments));
		}
		parsed->positions[parsed->count] = ix.ixCode;
		assignExpression(VAL, value);
		if (checkTypes && parsed->count < fixedParameters)
		{
			expectedType = signature->argpt[parsed->count].type;
			expectedPointers = signature->argpt[parsed->count].ptrs;
			(void)checkarg(value->ptrs,
			               value->type,
			               expectedPointers,
			               expectedType,
			               toString(signature->idName),
			               parsed->count);
		}
		parsed->arguments[parsed->count] =
		    semanticIrArgument(value, expectedType, expectedPointers);
		if (value->ptrs > 0 || value->fAddr || value->type != ID.T_DOUBLE)
		{
			outCode1(push_eax);
			parsed->stackBytes += 4;
		}
		else
		{
			outCode2(sub_esp, 8);
			outCode1(fstp_qsp);
			parsed->stackBytes += 8;
		}
		++parsed->count;
		if (!ispp(','))
		{
			skip(')');
			break;
		}
	}
	if (checkTypes &&
	    (parsed->count < fixedParameters || (!variadic && parsed->count > fixedParameters)))
	{
		error("funcCall", "too %s arguments", parsed->count < fixedParameters ? "few" : "many");
	}
	if (parsed->count > 1)
	{
		int move = ix.ixCode - parsed->positions[1];
		int destination;
		reallocCode(move + 1);
		memmove(&cd.pCode[parsed->positions[0] + move],
		        &cd.pCode[parsed->positions[0]],
		        (ix.ixCode - parsed->positions[0]) * sizeof(INSTRUCT));
		destination = parsed->positions[0];
		for (index = parsed->count; --index > 0;)
		{
			int size = (index + 1 < parsed->count ? parsed->positions[index + 1] : ix.ixCode) -
			           parsed->positions[index];
			memmove(&cd.pCode[destination],
			        &cd.pCode[move + parsed->positions[index]],
			        (size_t)size * sizeof(INSTRUCT));
			destination += size;
		}
	}
}

static void freeParsedCall(ParsedCall *parsed)
{
	free(parsed->positions);
	free(parsed->arguments);
	memset(parsed, 0, sizeof(*parsed));
}

static void functionCall(VALUE *value)
{
	int functionId = cd.token[ix.tix++].ival;
	Name *function = getNameFromTable(globTable, NM_FUNC, functionId);
	ParsedCall parsed;
	if (function == NULL)
	{
		char *name = toString(functionId);
		if (isupper((unsigned char)*name))
		{
			error("funcCall", "'%s' undeclared", name);
		}
		function = appendName(globTable, NM_CDECL, ID.T_INT, functionId, AD_CODE, 0);
	}
	parseCallArguments(function, value, &parsed);
	outCode2(call, function->idName);
	if ((function->type & NM_CDECL) != 0 && parsed.stackBytes > 0)
	{
		outCode2(add_esp, parsed.stackBytes);
	}
	setValue(VAL, function->ptrs, function->dataType, value);
	semanticIrCall(value, function, parsed.arguments, parsed.count);
	value->objectSize = sizeOfObjectType(function->dataType, function->ptrs);
	freeParsedCall(&parsed);
}

static void indirectFunctionCall(VALUE *value)
{
	const Name *signature = value->callable;
	IrValueId callee = value->irValue;
	ParsedCall parsed;
	if (signature == NULL || !signature->isFunctionPointer)
	{
		error("funcCall", "called object is not a function pointer");
	}
	outCode1(push_eax);
	parseCallArguments(signature, value, &parsed);
	outCode2(mov_eax_psp, parsed.stackBytes);
	outCode1(call_eax);
	if (signature->functionCallConvention == NM_WINAPI)
	{
		outCode2(add_esp, 4);
	}
	else
	{
		outCode2(add_esp, parsed.stackBytes + 4);
	}
	setValue(VAL, signature->returnPointers, signature->dataType, value);
	semanticIrCallIndirect(value, callee, signature, parsed.arguments, parsed.count);
	value->objectSize = sizeOfObjectType(signature->dataType, signature->returnPointers);
	freeParsedCall(&parsed);
}

/*============================================================================
 * sizeof
 *============================================================================*/

static void sizeOf(VALUE *pv)
{
	int size = 0;
	int parenthesized;
	ix.tix++;
	parenthesized = ispp('(');
	if (parenthesized && isTypeSpecifier(ix.tix))
	{
		int pointers = 0;
		typeSpecifier();
		while (ispp('*'))
		{
			++pointers;
		}
		size = sizeOfObjectType(var.type, pointers + getPtr(var.type));
		skip(')');
	}
	else
	{
		INDEX saved;
		VALUE operand;
		memcpy(&saved, &ix, sizeof(ix));
		irBuilderSuspend(&compiler.irBuilder);
		if (parenthesized)
		{
			expression(ADDR, &operand);
		}
		else
		{
			unaryExpression(ADDR, &operand);
		}
		irBuilderResume(&compiler.irBuilder);
		size = operand.ptrs > 0 && operand.objectSize > 0
		           ? operand.objectSize
		           : sizeOfObjectType(operand.type, operand.ptrs);
		ix.ixCode = saved.ixCode;
		ix.ixData = saved.ixData;
		ix.ixZero = saved.ixZero;
		ix.ixString = saved.ixString;
		if (parenthesized)
		{
			skip(')');
		}
	}
	outCode2(mov_eax, size);
	setValue(VAL, 0, ID.T_UINT, pv);
	pv->ival = size;
	pv->fConst = TRUE;
	semanticIrConstantInteger(pv);
}

/*============================================================================
 * Constant
 *============================================================================*/

static void constant(VALUE *pv)
{
	switch (cd.token[ix.tix].type)
	{
	case TK_STRING:
	{
		const char *literal = toString(cd.token[ix.tix].ival);
		loadAddr(AD_DATA, ix.ixData);
		ix.ixData += outString(ix.ixData, toString(cd.token[ix.tix++].ival)) + 1;
		setValue(VAL, 1, ID.T_CHAR, pv);
		semanticIrString(pv, literal);
		break;
	}
	case TK_FLOAT:
	case TK_DOUBLE:
		setValue(VAL, 0, cd.token[ix.tix].type == TK_FLOAT ? ID.T_FLOAT : ID.T_DOUBLE, pv);
		pv->rval = cd.token[ix.tix++].dval;
		pv->fConst = TRUE;
		outDataDouble(pv->rval);
		outCode3(fld_qp, ix.ixData - 8, AD_DATA);
		semanticIrConstantFloat(pv);
		break;
	case TK_INT:
		setValue(VAL, 0, ID.T_INT, pv);
		pv->ival = cd.token[ix.tix++].ival;
		pv->fConst = TRUE;
		outCode2(mov_eax, pv->ival);
		semanticIrConstantInteger(pv);
		break;
	case TK_CHAR:
		setValue(VAL, 0, ID.T_INT, pv);
		pv->ival =
		    cd.token[ix.tix].numericEscape
		        ? cd.token[ix.tix].ival
		        : cmd.target->encodeExecutionByte(&compiler, (unsigned char)cd.token[ix.tix].ival);
		++ix.tix;
		pv->fConst = TRUE;
		outCode2(mov_eax, pv->ival);
		semanticIrConstantInteger(pv);
		break;
	case TK_UINT:
	case TK_LONG:
	case TK_ULONG:
		setValue(VAL,
		         0,
		         cd.token[ix.tix].type == TK_UINT   ? ID.T_UINT
		         : cd.token[ix.tix].type == TK_LONG ? ID.T_LONG
		                                            : ID.T_ULONG,
		         pv);
		pv->ival = cd.token[ix.tix++].ival;
		pv->fConst = TRUE;
		outCode2(mov_eax, pv->ival);
		semanticIrConstantInteger(pv);
		break;
	default:
		error("expr.constant", "type=%d\n", cd.token[ix.tix].type);
	}
}

/*============================================================================
 * Primary Expression
 *============================================================================*/

static int pot[] = {0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 4};

static int parenthesizedExpressionMode(int requestedMode)
{
	int nesting = 1;
	int tokenIndex;

	for (tokenIndex = ix.tix; tokenIndex < cd.nToken; ++tokenIndex)
	{
		if (cd.token[tokenIndex].ival == '(')
		{
			++nesting;
		}
		else if (cd.token[tokenIndex].ival == ')' && --nesting == 0)
		{
			return tokenIndex + 1 < cd.nToken && cd.token[tokenIndex + 1].ival == '.'
			           ? ADDR
			           : requestedMode;
		}
	}
	return requestedMode;
}

static void primExpr(int mode, VALUE *pv)
{
	Name *pName = NULL;
	int ptrs, depth, type, size;
	int fInc = 0, fArray = 0, fDot = 0, fArrow = 0, fDone = 0;
	int hid = cd.token[ix.tix].ival;
	if (ispp('('))
	{
		expression(parenthesizedExpressionMode(mode), pv);
		skip(')');
		if (!is2("->") && !is('.') && !is('['))
		{
			return;
		}
		ptrs = pv->ptrs;
		type = pv->type;
		pName = pv->object;
		fDone = 1;
	}
	else
	{
		pName = getNameFromAllTable(cd.currTable, NM_VAR | NM_ENUM, hid);
		if (pName == NULL)
		{
			pName = getNameFromTable(globTable, NM_FUNC, hid);
		}
		if (pName == NULL)
		{
			error("primExpr", "'%s' undeclared", toString(hid));
		}
		ix.tix++;
		if (pName->type == NM_ENUM)
		{
			outCode2(mov_eax, pName->address);
			setValue(VAL, 0, ID.T_INT, pv);
			pv->ival = pName->address;
			pv->fConst = TRUE;
			semanticIrConstantInteger(pv);
			return;
		}
		type = pName->dataType;
		ptrs = pName->ptrs + getPtr(type);
		if (pName->addrType == AD_CODE)
		{
			loadAddr(AD_CODE, hid);
			setValue(VAL, ptrs + 1, type, pv);
			pv->fConst = TRUE;
			pv->fAddr = TRUE;
			pv->constantSymbol = pName->irSymbol;
			semanticIrNameAddress(pv, pName);
			pv->irValue = pv->irAddress;
			pv->irAddress = IR_VALUE_NONE;
			return;
		}
		loadAddr(pName->addrType, pName->addrType == AD_IMPORT ? hid : pName->address);
		setValue(ADDR, ptrs, type, pv);
		semanticIrNameAddress(pv, pName);
		pv->object = pName;
		if (pName->addrType == AD_DATA || pName->addrType == AD_ZERO ||
		    pName->addrType == AD_IMPORT)
		{
			pv->constantSymbol = pName->irSymbol;
		}
	}
	depth = pv->arrayDepth;
	while ((fArray = ispp('[')) || (fDot = ispp('.')) || (fArrow = is2pp("->")))
	{
		if (fArray)
		{
			VALUE baseValue;

			memcpy(&baseValue, pv, sizeof(baseValue));
			ptrs--;
			++depth;
			if (ptrs < 0 || depth >= 8)
			{
				error("primExpr", "subscript requires a pointer or array operand");
			}
			size = pName != NULL ? pName->size[depth] : 0;
			if (size == 0)
			{
				size = sizeOfObjectType(type, ptrs);
			}
			if ((pName == NULL || pName->size[depth] == 0) && baseValue.irValue == IR_VALUE_NONE)
			{
				semanticIrLoad(&baseValue);
				baseValue.irAddress = baseValue.irValue;
				baseValue.irValue = IR_VALUE_NONE;
				int *pI = &cd.pCode[ix.ixCode - 1].inst;
				if (*pI == lea_eax_pbp)
				{
					*pI = mov_eax_pbp;
				}
				else
				{
					outCode1(mov_eax_pax);
				}
			}
			int ixAddr = ix.ixCode - 1;
			outCode1(push_eax);
			int bgn = ix.ixCode;
			expression(VAL, pv);
			skip(']');
			semanticIrOffsetAddress(&baseValue, pv, size);
			pv->irAddress = baseValue.irAddress;
			pv->irValue = IR_VALUE_NONE;
			pv->mode = ADDR;
			pv->ptrs = ptrs;
			pv->type = type;
			if (pv->fConst)
			{
				ix.ixCode = bgn - 1;
				outCode2(add_eax, pv->ival * size);
				continue;
			}
			int fLocVar = ix.ixCode == bgn + 1 && cd.pCode[ix.ixCode - 1].inst == mov_eax_pbp;
			if (fLocVar)
			{
				delCode(bgn - 1);
				cd.pCode[ix.ixCode - 1].inst = mov_edx_pbp;
				if (size == 1 || (size <= 8 && pot[size] > 0))
				{
					outCode1(lea_eax_ad1 + pot[size]);
				}
				else
				{
					if (size > 1)
					{
						outCode2(imul_edx_edx, size);
					}
					outCode1(add_eax_edx);
				}
			}
			else
			{
				if (cd.pCode[ixAddr].inst == mov_eax_pbp)
				{
					outCode3(mov_edx_pbp, cd.pCode[ixAddr].num, cd.pCode[ixAddr].attr);
					delCodes(ixAddr, ixAddr + 2);
				}
				else
				{
					outCode1(pop_edx);
				}
				if (size == 1 || (size <= 8 && pot[size] > 0))
				{
					outCode1(lea_eax_da1 + pot[size]);
				}
				else
				{
					if (size > 1)
					{
						outCode2(imul_eax_eax, size);
					}
					outCode1(add_eax_edx);
				}
			}
		}
		else
		{
			int id = cd.token[ix.tix++].ival;
			if (fArrow)
			{
				if (pv->irValue == IR_VALUE_NONE)
				{
					semanticIrLoad(pv);
				}
				pv->irAddress = pv->irValue;
				pv->irValue = IR_VALUE_NONE;
			}
			pName = getAttr(type, id);
			if (pName == NULL)
			{
				error("prim", "'%s' not struct member", toString(id));
			}
			if (!fDone && fArrow)
			{
				outCode1(mov_eax_pax);
			}
			fDone = 0;
			fArrow = 0;
			ptrs = pName->ptrs;
			type = pName->dataType;
			depth = 0;
			outCode2(add_eax, pName->address);
			semanticIrAddAddressOffset(pv, pName->address);
			pv->mode = ADDR;
			pv->ptrs = ptrs;
			pv->type = type;
			pv->objectSize = pName->size[0];
			pv->callable = pName->isFunctionPointer ? pName : NULL;
		}
	}
	if (((fInc = is2pp("++")) || is2pp("--")) && mode == VAL)
	{
		pv->mode = ADDR;
		pv->ptrs = ptrs;
		pv->type = type;
		semanticIrIncrement(pv, fInc, TRUE);
		outCode1(mov_edx_eax);
		loadValue(type, ptrs > 0);
		incdec(type, ptrs, fInc, 'd');
		int k = ix.ixCode - 1;
		int inst = cd.pCode[k].inst;
		if (cd.pCode[k - 3].inst == lea_eax_pbp && cd.pCode[k - 2].inst == mov_edx_eax &&
		    cd.pCode[k - 1].inst == mov_eax_pax && cd.pCode[k].num == 1 &&
		    (inst == add_ddx || inst == sub_ddx))
		{
			cd.pCode[k - 3].inst = mov_eax_pbp;
			memcpy(&cd.pCode[k - 2], &cd.pCode[k - 3], sizeof(cd.pCode[k - 2]));
			cd.pCode[k - 2].inst = inst == add_ddx ? inc_dbp : dec_dbp;
			ix.ixCode -= 2;
		}
	}
	else if ((pName == NULL || (pName->arrays <= depth && pName->size[depth + 1] == 0)) &&
	         mode == VAL)
	{
		loadValue(type, ptrs > 0);
		pv->mode = ADDR;
		pv->ptrs = ptrs;
		pv->type = type;
		semanticIrLoad(pv);
		pv->constantSymbol = IR_SYMBOL_NONE;
	}
	else if (mode == VAL)
	{
		pv->irValue = pv->irAddress;
	}
	pv->mode = mode;
	pv->ptrs = ptrs;
	pv->type = type;
	pv->object = pName;
	pv->arrayDepth = depth;
	if (pName != NULL)
	{
		pv->objectSize = pName->size[depth] > 0 ? pName->size[depth] : sizeOfObjectType(type, ptrs);
		pv->callable = pName->isFunctionPointer ? pName : NULL;
	}
}

static int variadicBuiltin(VALUE *value)
{
	const char *name = toString(cd.token[ix.tix].ival);
	IrOpcode opcode;
	VALUE state;
	VALUE source;
	IrValueId right = IR_VALUE_NONE;
	int resultType = ID.T_VOID;
	int resultPointers = 0;
	if (strcmp(name, "__builtin_va_start") == 0)
	{
		opcode = IR_OP_VA_START;
	}
	else if (strcmp(name, "__builtin_va_arg") == 0)
	{
		opcode = IR_OP_VA_ARGUMENT;
	}
	else if (strcmp(name, "__builtin_va_copy") == 0)
	{
		opcode = IR_OP_VA_COPY;
	}
	else if (strcmp(name, "__builtin_va_end") == 0)
	{
		opcode = IR_OP_VA_END;
	}
	else
	{
		return FALSE;
	}
	++ix.tix;
	skip('(');
	assignExpression(VAL, &state);
	if (state.ptrs == 0)
	{
		error("stdarg", "variadic state requires an address");
	}
	if (opcode == IR_OP_VA_ARGUMENT)
	{
		skip(',');
		typeSpecifier();
		resultType = var.type;
		while (ispp('*'))
		{
			++resultPointers;
		}
		resultPointers += getPtr(resultType);
		if (resultPointers == 0 && (canonicalType(resultType) == ID.T_FLOAT ||
		                            (isIntegerType(resultType) && sizeOfDataType(resultType) < 4)))
		{
			error("stdarg", "va_arg must request the default-promoted argument type");
		}
	}
	else if (opcode != IR_OP_VA_END)
	{
		skip(',');
		assignExpression(VAL, &source);
		if (source.ptrs == 0)
		{
			error("stdarg", "variadic source requires an address");
		}
		if (opcode == IR_OP_VA_COPY)
		{
			right = source.irValue;
		}
		else if (compiler.irBuilder.function == NULL || !compiler.irBuilder.function->isVariadic)
		{
			error("stdarg", "va_start requires a variadic function definition");
		}
	}
	skip(')');
	setValue(VAL, resultPointers, resultType, value);
	if (semanticIrActive())
	{
		value->irValue = irBuilderEmitVariadic(&compiler.irBuilder,
		                                       opcode,
		                                       irTypeForCObject(resultType, resultPointers),
		                                       state.irValue,
		                                       right);
	}
	return TRUE;
}

static void primaryExpression(int mode, VALUE *pv)
{
	Name *namedObject = cd.token[ix.tix].type <= TK_NAME
	                        ? getNameFromAllTable(cd.currTable, NM_VAR, cd.token[ix.tix].ival)
	                        : NULL;

	if (cd.token[ix.tix].type == TK_NAME && variadicBuiltin(pv))
	{
		return;
	}
	if (cd.token[ix.tix].type >= TK_STRING)
	{
		constant(pv);
	}
	else if (is(ID.SIZEOF))
	{
		sizeOf(pv);
	}
	else if (!is('(') && isN('(', 1) && (namedObject == NULL || !namedObject->isFunctionPointer))
	{
		functionCall(pv);
		if (pv->type == ID.T_DOUBLE)
		{
			outCode1(fstp_st1);
		}
	}
	else
	{
		primExpr(mode, pv);
		if (is('('))
		{
			indirectFunctionCall(pv);
			if (pv->type == ID.T_DOUBLE)
			{
				outCode1(fstp_st1);
			}
		}
	}
}

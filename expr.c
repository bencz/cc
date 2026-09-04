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

static int integerResultType(const VALUE *left, const VALUE *right)
{
	if (left->ptrs > 0)
	{
		return left->type;
	}
	if (right->ptrs > 0)
	{
		return right->type;
	}
	return usualIntegerType(left->type, right->type);
}

static int unsignedIntegerOperation(const VALUE *left, const VALUE *right)
{
	return left->ptrs == 0 && right->ptrs == 0 &&
	       isUnsignedType(usualIntegerType(left->type, right->type));
}

static int comparisonInstruction(int op, int isUnsigned)
{
	if (!isUnsigned)
	{
		return op == '<'         ? setl_eax
		       : op == '>'       ? setg_eax
		       : op == id2("<=") ? setle_eax
		                         : setge_eax;
	}
	return op == '<' ? setb_eax : op == '>' ? seta_eax : op == id2("<=") ? setbe_eax : setae_eax;
}

/* Binary operators */

void infixOperation(int op, VALUE *v1, VALUE *v2, int lbgn, int lend)
{
	semanticIrBinary(op, v1, v2);
	if (op == FRONTEND_OP_SHIFT_LEFT)
	{
		op = id2("<<");
	}
	else if (op == FRONTEND_OP_SHIFT_RIGHT)
	{
		op = id2(">>");
	}
	int integerType =
	    isIntegerType(v1->type) && isIntegerType(v2->type) ? integerResultType(v1, v2) : ID.T_INT;
	int unsignedOperation =
	    isIntegerType(v1->type) && isIntegerType(v2->type) && unsignedIntegerOperation(v1, v2);
	if (v1->fConst && v2->fConst)
	{
		if (isIntegerType(v1->type) && isIntegerType(v2->type))
		{
			uint32_t left = (uint32_t)v1->ival;
			uint32_t right = (uint32_t)v2->ival;
			int comparison = FALSE;
			if (op == '+')
			{
				left += right;
			}
			else if (op == '-')
			{
				left -= right;
			}
			else if (op == '*')
			{
				left *= right;
			}
			else if (op == '/' || op == '%')
			{
				if (right == 0U)
				{
					error("infixOp", "division by zero in constant expression");
				}
				if (unsignedOperation)
				{
					left = op == '/' ? left / right : left % right;
				}
				else
				{
					int32_t signedLeft = (int32_t)left;
					int32_t signedRight = (int32_t)right;
					if (signedLeft == INT32_MIN && signedRight == -1)
					{
						error("infixOp", "signed division overflow in constant expression");
					}
					left =
					    (uint32_t)(op == '/' ? signedLeft / signedRight : signedLeft % signedRight);
				}
			}
			else if (op == '|')
			{
				left |= right;
			}
			else if (op == '&')
			{
				left &= right;
			}
			else if (op == '^')
			{
				left ^= right;
			}
			else if (op == id2("<<") || op == id2(">>"))
			{
				if (right >= 32U)
				{
					error("infixOp", "shift count is outside the 32-bit target range");
				}
				if (op == id2("<<"))
				{
					left <<= right;
				}
				else if (unsignedOperation)
				{
					left >>= right;
				}
				else
				{
					left = (uint32_t)((int32_t)left >> right);
				}
			}
			else if (op == id2("==") || op == id2("!="))
			{
				comparison = TRUE;
				left = op == id2("==") ? left == right : left != right;
			}
			else if (op == '<' || op == '>' || op == id2("<=") || op == id2(">="))
			{
				comparison = TRUE;
				if (unsignedOperation)
				{
					left = op == '<'         ? left < right
					       : op == '>'       ? left > right
					       : op == id2("<=") ? left <= right
					                         : left >= right;
				}
				else
				{
					int32_t signedLeft = (int32_t)left;
					int32_t signedRight = (int32_t)right;
					left = op == '<'         ? signedLeft < signedRight
					       : op == '>'       ? signedLeft > signedRight
					       : op == id2("<=") ? signedLeft <= signedRight
					                         : signedLeft >= signedRight;
				}
			}
			else
			{
				v1->fConst = FALSE;
			}
			if (v1->fConst)
			{
				v1->ival = (int)left;
				v1->type = comparison ? ID.T_INT : integerType;
			}
		}
		else if (v1->type == ID.T_DOUBLE || v2->type == ID.T_DOUBLE)
		{
			double d1 = v1->type == ID.T_DOUBLE ? v1->rval : (double)v1->ival;
			double d2 = v2->type == ID.T_DOUBLE ? v2->rval : (double)v2->ival;
			v1->type = ID.T_DOUBLE;
			if (op == '+')
			{
				v1->rval = d1 + d2;
			}
			else if (op == '-')
			{
				v1->rval = d1 - d2;
			}
			else if (op == '*')
			{
				v1->rval = d1 * d2;
			}
			else if (op == '/')
			{
				v1->rval = d1 / d2;
			}
			else
			{
				v1->fConst = FALSE;
			}
		}
		else
		{
			v1->fConst = FALSE;
		}
	}
	else if (v2->fConst && lend > 0 && isIntegerType(v1->type) && isIntegerType(v2->type))
	{

		if (v1->ptrs == 0 && (op == '+' || op == '-'))
		{
			ix.ixCode = lend;
			outCode2(op == '+' ? add_eax : sub_eax, v2->ival);
			return;
		}
		else if (op == '<' || op == '>' || op == id2("<=") || op == id2(">=") || op == id2("==") ||
		         op == id2("!="))
		{
			ix.ixCode = lend;
			outCode2(cmp_eax, v2->ival);
			if (unsignedOperation && op != id2("==") && op != id2("!="))
			{
				cd.pCode[ix.ixCode - 1].inst = ucmp_eax;
			}
			outCode1(op == id2("==")   ? sete_eax
			         : op == id2("!=") ? setne_eax
			                           : comparisonInstruction(op, unsignedOperation));
			return;
		}
		else if (op == '*')
		{
			ix.ixCode = lend;
			outCode2(imul_eax_eax, v2->ival);
			return;
		}
		else
		{
			v1->fConst = FALSE;
		}
	}
	else
	{
		v1->fConst = FALSE;
	}
	if ((v1->ptrs == 0 && v1->type == ID.T_DOUBLE) || (v2->ptrs == 0 && v2->type == ID.T_DOUBLE))
	{
		setFpuStack2(v1->type, v2->type);
		if (op == '=')
		{
			int fLV = (lend == lbgn + 1 && cd.pCode[lbgn].inst == lea_eax_pbp);
			if (fLV)
			{
				outCode3(fst_qbp, cd.pCode[lbgn].num, cd.pCode[lbgn].attr);
				delCodes(lbgn, lbgn + 2);
			}
			else
			{
				outCode1(pop_ecx);
				outCode1(fst_qcx);
			}
			outCode1(fstp_st1);
			v1->type = ID.T_DOUBLE;
		}
		else if (op == id2("==") || op == id2("!="))
		{
			outCode1(fucompp);
			outCode1(fstsw);
			outCode2(and_ah, 0x45);
			outCode2(op == id2("==") ? cmp_ah : xor_ah, 0x40);
			outCode1(op == id2("==") ? sete_eax : setne_eax);
			v1->type = ID.T_INT;
		}
		else if (op == '<' || op == '>' || op == id2("<=") || op == id2(">="))
		{
			if (op == '>' || op == id2(">="))
			{
				outCode1(fxch_st1);
			}
			outCode1(fucompp);
			outCode1(fstsw);
			outCode2(test_ah, (op == '<' || op == '>') ? 0x45 : 0x05);
			outCode1(sete_eax);
			v1->type = ID.T_INT;
		}
		else if (op == '+' || op == '-' || op == '*' || op == '/')
		{
			int *pI = &cd.pCode[ix.ixCode - 1].inst;
			if (*pI == fld_qbp)
			{
				*pI = op == '+' ? fadd_qbp : op == '-' ? fsub_qbp : op == '*' ? fmul_qbp : fdiv_qbp;
			}
			else if (*pI == fld_qp)
			{
				*pI = op == '+' ? fadd_qp : op == '-' ? fsub_qp : op == '*' ? fmul_qp : fdiv_qp;
			}
			else
			{
				outCode1(op == '+'   ? faddp_st1_st
				         : op == '-' ? fsubrp_st1_st
				         : op == '*' ? fmulp_st1_st
				                     : fdivrp_st1_st);
			}
			v1->type = ID.T_DOUBLE;
		}
		else
		{
			error("infixOp", "unsuported infixOp '%s'", toString(op));
		}
	}
	else
	{
		int n = sizeOfObjectType(v1->type, v1->ptrs);
		int fLV4 = (n == 4 && lend == lbgn + 1 && cd.pCode[lbgn].inst == lea_eax_pbp);
		if (fLV4 && (op == '=' || op == id2("+=") || op == id2("-=")))
		{
			int num = cd.pCode[lbgn].num;
			int attr = cd.pCode[lbgn].attr;
			int inst = op == '=' ? mov_pbp_eax : op == id2("+=") ? add_pbp_eax : sub_pbp_eax;
			delCodes(lbgn, lbgn + 2);
			outCode3(inst, num, attr);
			return;
		}
		if (op == '=' && canonicalType(v1->type) == ID.T_BOOL)
		{
			outCode1(test_eax_eax);
			outCode1(setne_eax);
		}
		int fECX = 1;
		if (lend > 0 && cd.pCode[lend].inst == push_eax)
		{
			int i;
			for (i = lend + 1; i < ix.ixCode && !(cd.pCode[i].regs & C); i++)
				;
			if (i == ix.ixCode)
			{
				fECX = 0;
			}
		}
		if (fECX)
		{
			outCode1(pop_ecx);
		}
		else
		{
			int *pI = &cd.pCode[lend - 1].inst;
			if (*pI == mov_eax)
			{
				*pI = mov_ecx;
				delCode(lend);
			}
			else if (*pI == mov_eax_pbp)
			{
				*pI = mov_ecx_pbp;
				delCode(lend);
			}
			else if (*pI == mov_eax_pax)
			{
				*pI = mov_ecx_pax;
				delCode(lend);
			}
			else if (*pI >= lea_eax_da1 && *pI <= lea_eax_da8)
			{
				*pI = lea_ecx_da1 + (*pI - lea_eax_da1);
				delCode(lend);
			}
			else
			{
				cd.pCode[lend].inst = mov_ecx_eax;
			}
		}
		if (op == '=')
		{
			outCode1(n == 4 ? mov_pcx_eax : n == 2 ? mov_pcx_ax : mov_pcx_al);
		}
		else if (op == id2("+="))
		{
			outCode1(n == 4 ? add_pcx_eax : n == 2 ? add_pcx_ax : add_pcx_al);
		}
		else if (op == id2("-="))
		{
			outCode1(n == 4 ? sub_pcx_eax : n == 2 ? sub_pcx_ax : sub_pcx_al);
		}
		else if (op == id2("&="))
		{
			outCode1(n == 4 ? and_pcx_eax : n == 2 ? and_pcx_ax : and_pcx_al);
		}
		else if (op == id2("|="))
		{
			outCode1(n == 4 ? or_pcx_eax : n == 2 ? or_pcx_ax : or_pcx_al);
		}
		else if (op == id2("^="))
		{
			outCode1(n == 4 ? xor_pcx_eax : n == 2 ? xor_pcx_ax : xor_pcx_al);
		}
		else if (op == id2("==") || op == id2("!="))
		{
			outCode1(cmp_eax_ecx);
			outCode1(op == id2("==") ? sete_eax : setne_eax);
		}
		else if (op == '<' || op == '>' || op == id2("<=") || op == id2(">="))
		{
			outCode1(unsignedOperation ? ucmp_ecx_eax : cmp_ecx_eax);
			outCode1(comparisonInstruction(op, unsignedOperation));
			v1->type = ID.T_INT;
		}
		else if (op == '+' || op == '-')
		{
			n = sizeOfObjectType(v1->type, v1->ptrs - 1);
			if (v1->ptrs > 0 && v2->ptrs == 0 && n > 1)
			{
				outCode2(imul_eax_eax, n);
			}
			if (op == '+')
			{
				outCode1(add_eax_ecx);
			}
			else
			{
				outCode1(xchg_eax_ecx);
				outCode1(sub_eax_ecx);
				if (v1->ptrs > 0 && v2->ptrs > 0)
				{
					outCode2(mov_ecx, n);
					outCode1(xdiv_ecx);
				}
			}
			return;
		}
		else if (op == '*')
		{
			int lastCode = ix.ixCode - 1;
			if (cd.pCode[lastCode - 1].inst == mov_ecx_pbp &&
			    cd.pCode[lastCode].inst == mov_eax_pbp)
			{
				cd.pCode[lastCode - 1].inst = mov_eax_pbp;
				cd.pCode[lastCode].inst = imul_eax_pbp;
			}
			else
			{
				outCode1(imul_eax_ecx);
			}
		}
		else if (op == '/' || op == '%')
		{
			outCode1(xchg_eax_ecx);
			outCode1(unsignedOperation ? (op == '/' ? udiv_ecx : umod_ecx)
			                           : (op == '/' ? xdiv_ecx : xmod_ecx));
		}
		else if (op == '|' || op == '&' || op == '^')
		{
			outCode1(op == '|' ? or_eax_ecx : op == '&' ? and_eax_ecx : xor_eax_ecx);
		}
		else if (op == id2("<<") || op == id2(">>"))
		{
			outCode1(xchg_eax_ecx);
			outCode1(op == id2("<<")
			             ? shl_eax_cl
			             : (isUnsignedType(integerPromotion(v1->type)) ? shr_eax_cl : sar_eax_cl));
		}
		else
		{
			error("infixOp", "unsuported infixOp '%s'", toString(op));
		}
		if (op != id2("==") && op != id2("!=") && op != '<' && op != '>' && op != id2("<=") &&
		    op != id2(">="))
		{
			v1->type = integerType;
		}
	}
}

/*============================================================================
 * Expression
 *============================================================================*/

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

void assignExpression(int mode, VALUE *pv)
{
	VALUE vr;
	INDEX ixSave;
	memcpy(&ixSave, &ix, sizeof(ix));
	irBuilderSuspend(&compiler.irBuilder);
	castExpression(ADDR, pv);
	irBuilderResume(&compiler.irBuilder);
	char *t = cd.token[ix.tix].token;
	if (strchr(";,])", *t) != NULL && pv->mode == VAL)
	{
		memcpy(&ix, &ixSave, sizeof(ix));
		conditionalExpression(mode, pv);
		return;
	}
	memcpy(&ix, &ixSave, sizeof(ix));
	int shiftAssign = strcmp(t, "<<=") == 0 || strcmp(t, ">>=") == 0;
	int fAssign = (*t == '=' && t[1] == '\0') || (strchr("+-/*%|&^", *t) != NULL && t[1] == '=') ||
	              shiftAssign;
	if (!fAssign)
	{
		conditionalExpression(mode, pv);
		return;
	}
	castExpression(ADDR, pv);
	if (pv->mode != ADDR)
	{
		error("assignExpr", "lvalue expected");
	}
	int op = cd.token[ix.tix].ival;
	t = cd.token[ix.tix++].token;
	int ixOp2 = ix.ixCode;
	outCode1(push_eax);
	if ((pv->ptrs > 0 || pv->type != ID.T_DOUBLE) && op != id2("*=") && op != id2("/=") &&
	    op != id2("%=") && !shiftAssign && !(canonicalType(pv->type) == ID.T_BOOL && t[1] != '\0'))
	{
		assignExpression(VAL, &vr);
		infixOperation(op, pv, &vr, ixSave.ixCode, ixOp2);
		int k = ix.ixCode - 1;
		if (op == '=' && cd.pCode[k - 1].inst == pop_ecx && cd.pCode[k].inst == mov_pcx_eax)
		{
			if (k - ixOp2 == 3 && cd.pCode[ixOp2 + 1].inst == mov_eax)
			{
				cd.pCode[ixOp2 + 1].inst = mov_dax;
				delCode(k);
				delCode(k - 1);
				delCode(ixOp2);
			}
		}
		return;
	}
	if (t[1] != '\0')
	{
		int size = sizeOfObjectType(pv->type, pv->ptrs);
		if (size == 8)
		{
			outCode1(fld_qax);
		}
		else
		{
			outCode1(size == 4   ? mov_eax_pax
			         : size == 2 ? (isUnsignedType(pv->type) ? movzx_eax_wax : movsx_eax_wax)
			                     : (isUnsignedType(pv->type) ? movzx_eax_bax : movsx_eax_bax));
			outCode1(push_eax);
		}
	}
	assignExpression(VAL, &vr);
	if (t[0] != '=')
	{
		VALUE assignmentTarget;

		memcpy(&assignmentTarget, pv, sizeof(assignmentTarget));
		infixOperation(shiftAssign ? id2(t) : t[0], pv, &vr, -1, -1);
		pv->type = assignmentTarget.type;
		pv->ptrs = assignmentTarget.ptrs;
		pv->objectSize = assignmentTarget.objectSize;
		pv->irAddress = assignmentTarget.irAddress;
		infixOperation('=', pv, &vr, -1, -1);
	}
	else
	{
		infixOperation('=', pv, &vr, ixSave.ixCode, ixOp2);
	}
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
		pv->type = ID.T_INT;
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
		pv->type = ID.T_INT;
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

static void castExpression(int mode, VALUE *pv)
{
	if (!is('(') || !(isTypeSpecifier(ix.tix + 1) && !isN('(', 2)))
	{
		unaryExpression(mode, pv);
		return;
	}
	++ix.tix;
	typeSpecifier();
	int idCastType = var.type;
	int ptr = getPtr(idCastType);
	for (; ispp('*');)
	{
		ptr++;
	}
	skip(')');
	castExpression(mode, pv);
	if (ptr > 0)
	{
		;
	}
	else if (canonicalType(idCastType) == ID.T_VOID)
	{
		/* The operand is still evaluated; only its resulting value is discarded. */
	}
	else if (canonicalType(idCastType) == ID.T_BOOL && pv->type == ID.T_DOUBLE)
	{
		outDataDouble(0.0);
		outCode3(fld_qp, ix.ixData - 8, AD_DATA);
		outCode1(fucompp);
		outCode1(fstsw);
		outCode2(and_ah, 0x45);
		outCode2(cmp_ah, 0x40);
		outCode1(setne_eax);
		if (pv->fConst)
		{
			pv->ival = pv->rval != 0.0;
		}
	}
	else if (isIntegerType(idCastType) && pv->type == ID.T_DOUBLE)
	{
		outCode3(fldcw, 2, AD_DATA);
		if (isUnsignedType(idCastType))
		{
			outCode1(fistp_ueax);
		}
		else
		{
			outCode2(fistp_dsp, -4);
			outCode2(mov_eax_psp, -4);
		}
		outCode3(fldcw, 0, AD_DATA);
		if (pv->fConst)
		{
			pv->ival = isUnsignedType(idCastType) ? (int)(uint32_t)pv->rval : (int32_t)pv->rval;
		}
	}
	else if (isIntegerType(idCastType) && (isIntegerType(pv->type) || pv->ptrs > 0))
	{
		int width = sizeOfDataType(idCastType);
		if (idCastType == ID.T_BOOL)
		{
			outCode1(test_eax_eax);
			outCode1(setne_eax);
			if (pv->fConst)
			{
				pv->ival = !!pv->ival;
			}
		}
		else if (width < 4)
		{
			int shift = width == 1 ? 24 : 16;
			outCode2(shl_eax, shift);
			outCode2(isUnsignedType(idCastType) ? shr_eax : sar_eax, shift);
			if (pv->fConst)
			{
				uint32_t value = (uint32_t)pv->ival;
				value &= width == 1 ? UINT8_MAX : UINT16_MAX;
				if (!isUnsignedType(idCastType))
				{
					value = width == 1 ? (uint32_t)(int32_t)(int8_t)value
					                   : (uint32_t)(int32_t)(int16_t)value;
				}
				pv->ival = (int)value;
			}
		}
	}
	else if (idCastType == ID.T_DOUBLE && pv->type != ID.T_DOUBLE)
	{
		if (!isIntegerType(pv->type))
		{
			error("castExpr", "conversion to double requires an arithmetic operand");
		}
		if (isUnsignedType(integerPromotion(pv->type)))
		{
			outCode1(fild_uax);
		}
		else
		{
			outCode2(mov_psp_eax, -4);
			outCode2(fild_dsp, -4);
		}
		if (pv->fConst)
		{
			pv->rval = isUnsignedType(integerPromotion(pv->type)) ? (double)(uint32_t)pv->ival
			                                                      : (double)pv->ival;
		}
	}
	else if (idCastType != pv->type)
	{
		error("castExpr",
		      "unsupported cast(casttype=%s type=%s)\n",
		      toString(idCastType),
		      toString(pv->type));
	}
	semanticIrCast(pv, idCastType, ptr);
	pv->ptrs = ptr;
	pv->type = idCastType;
	pv->objectSize = sizeOfObjectType(idCastType, ptr);
}

/*============================================================================
 * Unary Expression
 *============================================================================*/

static void unaryExpression(int mode, VALUE *pv)
{
	int fNEG = 0, fNOT = 0, fBITNOT = 0, fINC = 0;
	if (ispp('+') || (fNEG = ispp('-')) || (fNOT = ispp('!')) || (fBITNOT = ispp('~')))
	{
		int bgn = ix.ixCode;
		castExpression(VAL, pv);
		if (fNEG)
		{
			if (pv->fConst && isIntegerType(pv->type))
			{
				cd.pCode[ix.ixCode - 1].num = (pv->ival = -pv->ival);
			}
			else if (pv->fConst && pv->type == ID.T_DOUBLE)
			{
				cd.pCode[bgn].dval = (pv->rval = -pv->rval);
			}
			else
			{
				outCode1(pv->type == ID.T_DOUBLE ? fchs : neg_eax);
			}
		}
		else if (fNOT)
		{
			if (pv->fConst)
			{
				pv->ival = !pv->ival;
			}
			outCode1(test_eax_eax);
			outCode1(sete_eax);
		}
		else if (fBITNOT)
		{
			if (!isIntegerType(pv->type))
			{
				error("unaryExpr", "integer operand required for '~'");
			}
			pv->type = integerPromotion(pv->type);
			if (pv->fConst)
			{
				pv->ival = ~pv->ival;
			}
			outCode1(not_eax);
		}
		semanticIrUnary(fNEG ? '-' : fNOT ? '!' : fBITNOT ? '~' : '+', pv);
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
	case TK_DOUBLE:
		setValue(VAL, 0, ID.T_DOUBLE, pv);
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
	}
	depth = 0;
	while ((fArray = ispp('[')) || (fDot = ispp('.')) || (fArrow = is2pp("->")))
	{
		if (fArray)
		{
			VALUE baseValue;

			memcpy(&baseValue, pv, sizeof(baseValue));
			ptrs--;
			size = pName->size[++depth];
			if (size == 0)
			{
				size = sizeOfObjectType(pName->dataType, ptrs);
			}
			if (pName->size[depth] == 0)
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
	else if (pName->arrays <= depth && pName->size[depth + 1] == 0 && mode == VAL)
	{
		loadValue(type, ptrs > 0);
		pv->mode = ADDR;
		pv->ptrs = ptrs;
		pv->type = type;
		semanticIrLoad(pv);
	}
	else if (mode == VAL)
	{
		pv->irValue = pv->irAddress;
	}
	pv->mode = mode;
	pv->ptrs = ptrs;
	pv->type = type;
	if (pName != NULL)
	{
		pv->objectSize = pName->size[depth] > 0 ? pName->size[depth] : sizeOfObjectType(type, ptrs);
		pv->callable = pName->isFunctionPointer ? pName : NULL;
	}
}

static void primaryExpression(int mode, VALUE *pv)
{
	Name *namedObject = cd.token[ix.tix].type <= TK_NAME
	                        ? getNameFromAllTable(cd.currTable, NM_VAR, cd.token[ix.tix].ival)
	                        : NULL;

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

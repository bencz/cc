/*
 * expr.c - Processamento de Expressões
 * 
 * Responsável pela análise e geração de código para expressões
 * aritméticas, lógicas, relacionais e de atribuição.
 */

#include "cc.h"

/*============================================================================
 * Operações Infix
 *============================================================================*/

void infixOperation(int op, VALUE *v1, VALUE *v2, int lbgn, int lend)
{
	if (v1->fConst && v2->fConst)
	{
		if (v1->type == ID.INT && v2->type == ID.INT)
		{
			if (op == '+') v1->ival += v2->ival;
			else if (op == '-') v1->ival -= v2->ival;
			else if (op == '*') v1->ival *= v2->ival;
			else if (op == '/') v1->ival /= v2->ival;
			else if (op == '%') v1->ival %= v2->ival;
			else if (op == '|') v1->ival |= v2->ival;
			else if (op == '&') v1->ival &= v2->ival;
			else if (op == '^') v1->ival ^= v2->ival;
			else v1->fConst = FALSE;
		}
		else if (v1->type == ID.DOUBLE || v2->type == ID.DOUBLE)
		{
			double d1 = v1->type == ID.DOUBLE ? v1->rval : (double)v1->ival;
			double d2 = v2->type == ID.DOUBLE ? v2->rval : (double)v2->ival;
			v1->type = ID.DOUBLE;
			if (op == '+') v1->rval = d1 + d2;
			else if (op == '-') v1->rval = d1 - d2;
			else if (op == '*') v1->rval = d1 * d2;
			else if (op == '/') v1->rval = d1 / d2;
			else v1->fConst = FALSE;
		}
		else
		{
			v1->fConst = FALSE;
		}
	}
	else if (v2->fConst && lend > 0 && v1->type == ID.INT && v2->type == ID.INT)
	{

		if (v1->ptrs == 0 && (op == '+' || op == '-'))
		{
			ix.ixCode = lend;
			outCode2(op == '+' ? add_eax : sub_eax, v2->ival);
			return;
		}
		else if (op == '<' || op == '>' || op == id2("<=") || op == id2(">=") || op == id2("==") || op == id2("!="))
		{
			ix.ixCode = lend;
			outCode2(cmp_eax, v2->ival);
			outCode1(op == '<' ? setl_eax : op == '>' ? setg_eax : op == id2("<=") ? setle_eax :
				op == id2(">=") ? setge_eax : op == id2("==") ? sete_eax : setne_eax);
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
	if ((v1->ptrs == 0 && v1->type == ID.DOUBLE) || (v2->ptrs == 0 && v2->type == ID.DOUBLE))
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
			v1->type = ID.DOUBLE;
		}
		else if (op == id2("==") || op == id2("!="))
		{
			outCode1(fucompp);
			outCode1(fstsw);
			outCode2(and_ah, 0x45);
			outCode2(op == id2("==") ? cmp_ah : xor_ah, 0x40);
			outCode1(op == id2("==") ? sete_eax : setne_eax);
			v1->type = ID.INT;
		}
		else if (op == '<' || op == '>' || op == id2("<=") || op == id2(">="))
		{
			if (op == '>' || op == id2(">=")) outCode1(fxch_st1);
			outCode1(fucompp);
			outCode1(fstsw);
			outCode2(test_ah, (op == '<' || op == '>') ? 0x45 : 0x05);
			outCode1(sete_eax);
			v1->type = ID.INT;
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
				outCode1(op == '+' ? faddp_st1_st : op == '-' ? fsubrp_st1_st :
					op == '*' ? fmulp_st1_st : fdivrp_st1_st);
			}
			v1->type = ID.DOUBLE;
		}
		else
		{
			error("infixOp", "unsuported infixOp '%s'", toString(op));
		}
	}
	else
	{
		int n = v1->ptrs > 0 ? 4 : sizeOfDataType(v1->type);
		int k = ix.ixCode - 1;
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
		int fECX = 1;
		if (lend > 0 && cd.pCode[lend].inst == push_eax)
		{
			int i;
			for (i = lend + 1; i < ix.ixCode && !(cd.pCode[i].regs & C); i++);
			if (i == ix.ixCode) fECX = 0;
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
		if (op == '=') outCode1(n == 4 ? mov_pcx_eax : n == 2 ? mov_pcx_ax : mov_pcx_al);
		else if (op == id2("+=")) outCode1(n == 4 ? add_pcx_eax : n == 2 ? add_pcx_ax : add_pcx_al);
		else if (op == id2("-=")) outCode1(n == 4 ? sub_pcx_eax : n == 2 ? sub_pcx_ax : sub_pcx_al);
		else if (op == id2("&=")) outCode1(n == 4 ? and_pcx_eax : n == 2 ? and_pcx_ax : and_pcx_al);
		else if (op == id2("|=")) outCode1(n == 4 ? or_pcx_eax : n == 2 ? or_pcx_ax : or_pcx_al);
		else if (op == id2("^=")) outCode1(n == 4 ? xor_pcx_eax : n == 2 ? xor_pcx_ax : xor_pcx_al);
		else if (op == id2("==") || op == id2("!="))
		{
			outCode1(cmp_eax_ecx);
			outCode1(op == id2("==") ? sete_eax : setne_eax);
		}
		else if (op == '<' || op == '>' || op == id2("<=") || op == id2(">="))
		{
			outCode1(cmp_ecx_eax);
			outCode1(op == '<' ? setl_eax : op == '>' ? setg_eax : op == id2("<=") ? setle_eax : setge_eax);
		}
		else if (op == '+' || op == '-')
		{
			n = v1->ptrs > 1 ? 4 : sizeOfDataType(v1->type);
			if (v1->ptrs > 0 && v2->ptrs == 0 && n > 1) outCode2(imul_eax_eax, n);
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
			int k = ix.ixCode - 1;
			if (cd.pCode[k - 1].inst == mov_ecx_pbp && cd.pCode[k].inst == mov_eax_pbp)
			{
				cd.pCode[k - 1].inst = mov_eax_pbp;
				cd.pCode[k].inst = imul_eax_pbp;
			}
			else
			{
				outCode1(imul_eax_ecx);
			}
		}
		else if (op == '/' || op == '%')
		{
			outCode1(xchg_eax_ecx);
			outCode1(op == '/' ? xdiv_ecx : xmod_ecx);
		}
		else if (op == '|' || op == '&' || op == '^')
		{
			outCode1(op == '|' ? or_eax_ecx : op == '&' ? and_eax_ecx : xor_eax_ecx);
		}
		else if (op == id2("<<") || op == id2(">>"))
		{
			outCode1(xchg_eax_ecx);
			outCode1(op == id2("<<") ? shl_eax_cl : shr_eax_cl);
		}
		else
		{
			error("infixOp", "unsuported infixOp '%s'", toString(op));
		}
		v1->type = ID.INT;
	}
}

/*============================================================================
 * Expression
 *============================================================================*/

void expression(int mode, VALUE *pv)
{
	assignExpression(mode, pv);
	while (ispp(',')) assignExpression(mode, pv);
}

/*============================================================================
 * Assign Expression
 *============================================================================*/

void assignExpression(int mode, VALUE *pv)
{
	VALUE vr;
	INDEX ixSave = ix;
	castExpression(ADDR, pv);
	char *t = cd.token[ix.tix].token;
	if (strchr(";,])", *t) != NULL && pv->mode == VAL) return;
	ix = ixSave;
	int fAssign = (*t == '=' && t[1] == '\0') || (strchr("+-/*%|&^", *t) != NULL && t[1] == '=');
	if (!fAssign)
	{
		conditionalExpression(mode, pv);
		return;
	}
	castExpression(ADDR, pv);
	if (pv->mode != ADDR) error("assignExpr", "lvalue expected");
	int op = cd.token[ix.tix].ival;
	t = cd.token[ix.tix++].token;
	int ixOp2 = ix.ixCode;
	outCode1(push_eax);
	if ((pv->ptrs > 0 || pv->type != ID.DOUBLE)
		&& op != id2("*=") && op != id2("/=") && op != id2("%="))
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
		int size = pv->ptrs > 0 ? 4 : sizeOfDataType(pv->type);
		if (size == 8)
		{
			outCode1(fld_qax);
		}
		else
		{
			outCode1(size == 4 ? mov_eax_pax : size == 2 ? movsx_eax_wax : movsx_eax_bax);
			outCode1(push_eax);
		}
	}
	assignExpression(VAL, &vr);
	if (t[0] != '=')
	{
		infixOperation(t[0], pv, &vr, -1, -1);
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
	INDEX ixBgn = ix;
	logicalOrExpression(mode, pv);
	if (ispp('?'))
	{
		outCode1(test_eax_eax);
		outCode2(jz, locFalse = loc());
		INSTRUCT *pC = &cd.pCode[ix.ixCode - 3];
		if (pC[0].inst == sete_eax && pC[1].inst == test_eax_eax && pC[2].inst == jz)
		{
			pC[2].inst = jnz;
			delCodes(ix.ixCode - 3, ix.ixCode - 1);
		}
		assignExpression(VAL, pv);
		outCode2(jmp, locEnd = loc());
		skip(':');
		outCode2(loc_, locFalse);
		assignExpression(VAL, pv);
		outCode2(loc_, locEnd);
		pv->fConst = FALSE;
	}
	if (pv->fConst)
	{
		int tixCurr = ix.tix;
		ix = ixBgn;
		ix.tix = tixCurr;
		if (pv->ptrs > 0)
		{
			outCode3(mov_eax, pv->ival, AD_CONST);
		}
		else if (pv->type == ID.INT)
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

void logicalOrExpression(int mode, VALUE *pv)
{
	logicalAndExpression(mode, pv);
	if (!is2("||")) return;
	int locTRUE = loc();
	int locNext = loc();
	while (is2pp("||"))
	{
		jumpTrue(locTRUE, "||");
		logicalAndExpression(mode, pv);
		pv->type = ID.INT;
	}
	jumpTrue(locTRUE, "||");
	outCode2(mov_eax, 0);
	outCode2(jmp, locNext);
	outCode2(loc_, locTRUE);
	outCode2(mov_eax, 1);
	outCode2(loc_, locNext);
}

/*============================================================================
 * Logical And Expression
 *============================================================================*/

void logicalAndExpression(int mode, VALUE *pv)
{
	orExpression(mode, pv);
	if (!is2("&&")) return;
	int locFalse = loc();
	int locNext = loc();
	while (is2pp("&&"))
	{
		jumpFalse(locFalse, "&&");
		orExpression(mode, pv);
		pv->type = ID.INT;
	}
	jumpFalse(locFalse, "&&");
	outCode2(mov_eax, 1);
	outCode2(jmp, locNext);
	outCode3(loc_, locFalse, 'F');
	outCode2(mov_eax, 0);
	outCode3(loc_, locNext, 'N');
}

/*============================================================================
 * Or Expression
 *============================================================================*/

void orExpression(int mode, VALUE *pv)
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

void xorExpression(int mode, VALUE *pv)
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

void andExpression(int mode, VALUE *pv)
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

void equalityExpression(int mode, VALUE *pv)
{
	int fEQ;
	relationalExpression(mode, pv);
	if ((fEQ = is2pp("==")) || is2pp("!="))
	{
		VALUE v2;
		int ixOp2 = ix.ixCode;
		if (pv->type != ID.DOUBLE) outCode1(push_eax);
		relationalExpression(mode, &v2);
		if (ix.ixCode - ixOp2 == 2 && cd.pCode[ix.ixCode - 1].inst == mov_eax_pbp)
		{
			cd.pCode[ix.ixCode - 1].inst = cmp_eax_pbp;
			outCode1(fEQ ? sete_eax : setne_eax);
			delCode(ixOp2);
		}
		else
		{
			infixOperation(fEQ ? id2("==") : id2("!="), pv, &v2, -1, ixOp2);
		}
	}
}

/*============================================================================
 * Relational Expression
 *============================================================================*/

void relationalExpression(int mode, VALUE *pv)
{
	int fLT = 0, fGT = 0, fLE = 0;
	shiftExpression(mode, pv);
	if ((fLT = ispp('<')) || (fGT = ispp('>')) || (fLE = is2pp("<=")) || is2pp(">="))
	{
		VALUE v2;
		int ixOp2 = ix.ixCode;
		if (pv->type != ID.DOUBLE) outCode1(push_eax);
		shiftExpression(mode, &v2);
		if (ix.ixCode - ixOp2 == 2 && cd.pCode[ix.ixCode - 1].inst == mov_eax_pbp)
		{
			cd.pCode[ix.ixCode - 1].inst = cmp_eax_pbp;
			outCode1(fLT ? setl_eax : fGT ? setg_eax : fLE ? setle_eax : setge_eax);
			delCode(ixOp2);
		}
		else
		{
			infixOperation(fLT ? '<' : fGT ? '>' : fLE ? id2("<=") : id2(">="), pv, &v2, -1, ixOp2);
		}
	}
}

/*============================================================================
 * Shift Expression
 *============================================================================*/

void shiftExpression(int mode, VALUE *pv)
{
	int fShl;
	addExpression(mode, pv);
	if ((fShl = is2pp("<<")) || is2pp(">>"))
	{
		VALUE v2;
		int ixOp2 = ix.ixCode;
		outCode1(push_eax);
		addExpression(mode, &v2);
		infixOperation(fShl ? id2("<<") : id2(">>"), pv, &v2, -1, -1);
	}
}

/*============================================================================
 * Add Expression
 *============================================================================*/

void addExpression(int mode, VALUE *pv)
{
	int fAdd;
	mulExpression(mode, pv);
	while ((fAdd = ispp('+')) || ispp('-'))
	{
		VALUE v2;
		int ixOp2 = ix.ixCode;
		if (pv->type != ID.DOUBLE) outCode1(push_eax);
		mulExpression(mode, &v2);
		infixOperation(fAdd ? '+' : '-', pv, &v2, -1, ixOp2);
	}
}

/*============================================================================
 * Mul Expression
 *============================================================================*/

void mulExpression(int mode, VALUE *pv)
{
	int fMul = 0, fDiv = 0;
	castExpression(mode, pv);
	while ((fMul = ispp('*')) || (fDiv = ispp('/')) || ispp('%'))
	{
		VALUE v2;
		int ixOp2 = ix.ixCode;
		if (pv->type != ID.DOUBLE) outCode1(push_eax);
		castExpression(mode, &v2);
		infixOperation(fMul ? '*' : fDiv ? '/' : '%', pv, &v2, -1, ixOp2);
	}
}

/*============================================================================
 * Cast Expression
 *============================================================================*/

void castExpression(int mode, VALUE *pv)
{
	if (!is('(') || !(isTypeSpecifier(ix.tix + 1) && !isN('(', 2)))
	{
		unaryExpression(mode, pv);
		return;
	}
	int idCastType = cd.token[++ix.tix].ival;
	int ptr = getPtr(idCastType);
	for (ix.tix++; ispp('*');) ptr++;
	skip(')');
	unaryExpression(mode, pv);
	if (ptr > 0)
	{
		;
	}
	else if (idCastType == ID.INT && pv->type == ID.DOUBLE)
	{
		outCode3(fldcw, 2, AD_DATA);
		outCode2(fistp_dsp, -4);
		outCode2(mov_eax_psp, -4);
		outCode3(fldcw, 0, AD_DATA);
		if (pv->fConst) pv->ival = (int)pv->rval;
	}
	else if (idCastType == ID.SHORT && pv->type == ID.INT)
	{
		outCode1(cwde);
		if (pv->fConst) pv->ival = (short)pv->ival;
	}
	else if (idCastType == ID.DOUBLE && pv->type != ID.DOUBLE)
	{
		outCode2(mov_psp_eax, -4);
		outCode2(fild_dsp, -4);
		if (pv->fConst) pv->rval = (double)pv->ival;
	}
	else if (idCastType != ID.INT)
	{
		error("castExpr", "unsupported cast(casttype=%s type=%s)\n",
			toString(idCastType), toString(pv->type));
	}
	pv->ptrs = ptr;
	pv->type = idCastType;
}

/*============================================================================
 * Unary Expression
 *============================================================================*/

void unaryExpression(int mode, VALUE *pv)
{
	int fNEG = 0, fNOT = 0, fINC = 0;
	if (ispp('+') || (fNEG = ispp('-')) || (fNOT = ispp('!')))
	{
		int bgn = ix.ixCode;
		primaryExpression(VAL, pv);
		if (fNEG)
		{
			if (pv->fConst && pv->type == ID.INT)
			{
				cd.pCode[ix.ixCode - 1].num = (pv->ival = -pv->ival);
			}
			else if (pv->fConst && pv->type == ID.DOUBLE)
			{
				cd.pCode[bgn].dval = (pv->rval = -pv->rval);
			}
			else
			{
				outCode1(pv->type == ID.DOUBLE ? fchs : neg_eax);
			}
		}
		else if (fNOT)
		{
			if (pv->fConst) pv->ival = !pv->ival;
			outCode1(test_eax_eax);
			outCode1(sete_eax);
		}
	}
	else if ((fINC = is2pp("++")) || is2pp("--"))
	{
		primaryExpression(ADDR, pv);
		incdec(pv->type, pv->ptrs > 0, fINC, 'a');
		loadValue(pv->type, pv->ptrs > 0);
	}
	else if (ispp('&'))
	{
		primaryExpression(ADDR, pv);
		pv->fAddr = TRUE;
		pv->ptrs++;
	}
	else if (ispp('*'))
	{
		primaryExpression(VAL, pv);
		pv->mode = ADDR;
		pv->ptrs--;
		if (mode == VAL) loadValue(pv->type, pv->ptrs > 0);
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
	if (type2 == ID.DOTS3) return FALSE;
	if (pt1 > 0 && pt2 == 0) error("chk", f, name, nArg, "integer", "pointer");
	if (pt1 == 0 && pt2 > 0) error("chk", f, name, nArg, "pointer", "integer");
	if (pt1 != pt2 && type1 != ID.HVOID && type2 != ID.HVOID) error("chk", g, name, nArg);
	return TRUE;
}

static void functionCall(VALUE *pv)
{
	int n, nSize, nP, p[20], depth = 0;

	int fid = cd.token[ix.tix++].ival;
	Name *pName = getNameFromTable(globTable, NM_FUNC, fid);
	if (pName == NULL)
	{
		char *func = toString(fid);
		if (isupper(*func)) error("funcCall", "'%s' undeclared", func);
		pName = appendName(globTable, NM_CDECL, ID.INT, fid, AD_CODE, 0);
	}
	skip('(');
	int fCheck = (pName->argc >= 0 && fid != id("main"));
	for (nP = 0; !ispp(')'); nP++)
	{
		p[nP] = ix.ixCode;
		assignExpression(VAL, pv);
		if (fCheck && nP < pName->argc)
			fCheck = checkarg(pv->ptrs, pv->type, pName->argpt[nP].ptrs,
			pName->argpt[nP].type, toString(pName->idName), nP);
		if (pv->ptrs > 0 || pv->fAddr || pv->type != ID.DOUBLE)
		{
			outCode1(push_eax);
			depth += 4;
		}
		else
		{
			outCode2(sub_esp, 8);
			outCode1(fstp_qsp);
			depth += 8;
		}
		ispp(',');
	}
	int diff = pName->argc - nP;
	if (diff > 0 && pName->argpt[nP].type == ID.DOTS3) fCheck = FALSE;
	if (fCheck && diff != 0)
		error("funcCall", "too %s arguments", diff < 0 ? "many" : "few");
	if (nP > 1)
	{
		int mv = ix.ixCode - p[1];
		reallocCode(mv + 1);
		memmove(&cd.pCode[*p + mv], &cd.pCode[*p], (ix.ixCode - *p)*sizeof(INSTRUCT));
		int dst = p[0];
		for (n = nP; --n > 0; dst += nSize)
		{
			nSize = (n + 1 < nP ? p[n + 1] : ix.ixCode) - p[n];
			memmove(&cd.pCode[dst], &cd.pCode[mv + p[n]], nSize*sizeof(INSTRUCT));
		}
	}
	outCode2(call, pName->idName);
	if ((pName->type&NM_CDECL) && depth > 0)
		outCode2(add_esp, depth);
	setValue(VAL, pName->ptrs, pName->dataType, pv);
}

/*============================================================================
 * sizeof
 *============================================================================*/

static void sizeOf(VALUE *pv)
{
	int cnt = 0, pointers = 0;
	ix.tix++;
	while (ispp('(')) cnt++;
	int id = cd.token[ix.tix++].ival;
	while (ispp('*')) pointers++;
	int size = pointers > 0 ? 4 : sizeOfDataType(id);
	if (size == 0)
	{
		Name *pName = getNameFromAllTable(cd.currTable, NM_VAR, id);
		if (pName != NULL) size = pName->size[0];
	}
	outCode2(mov_eax, size);
	while (cnt-- > 0) skip(')');
	setValue(VAL, 0, ID.INT, pv);
	pv->ival = size;
	pv->fConst = TRUE;
}

/*============================================================================
 * Constant
 *============================================================================*/

static void constant(VALUE *pv)
{
	switch (cd.token[ix.tix].type)
	{
	case TK_STRING:
		loadAddr(AD_DATA, ix.ixData);
		ix.ixData += outString(ix.ixData, toString(cd.token[ix.tix++].ival)) + 1;
		setValue(VAL, 1, ID.CHAR, pv);
		break;
	case TK_DOUBLE:
		setValue(VAL, 0, ID.DOUBLE, pv);
		pv->rval = cd.token[ix.tix++].dval;
		pv->fConst = TRUE;
		outDataDouble(pv->rval);
		outCode3(fld_qp, ix.ixData - 8, AD_DATA);
		break;
	case TK_INT:
	case TK_CHAR:
		setValue(VAL, 0, ID.INT, pv);
		pv->ival = cd.token[ix.tix++].ival;
		pv->fConst = TRUE;
		outCode2(mov_eax, pv->ival);
		break;
	default:
		error("expr.constant", "type=%d\n", cd.token[ix.tix].type);
	}
}

/*============================================================================
 * Primary Expression
 *============================================================================*/

static int pot[] = { 0, 0, 1, 0, 2, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 4 };

static void primExpr(int mode, VALUE *pv)
{
	Name *pName = NULL;
	int ptrs, depth, type, size;
	int fInc = 0, fArray = 0, fDot = 0, fArrow = 0, fDone = 0;
	int hid = cd.token[ix.tix].ival;
	if (ispp('('))
	{
		expression(mode, pv);
		skip(')');
		if (!is2("->")) return;
		ptrs = pv->ptrs;
		type = pv->type;
		fDone = 1;
	}
	else
	{
		pName = getNameFromAllTable(cd.currTable, NM_VAR | NM_ENUM, hid);
		if (pName == NULL)
			pName = getNameFromTable(globTable, NM_FUNC, hid);
		if (pName == NULL)
			error("primExpr", "'%s' undeclared", toString(hid));
		ix.tix++;
		if (pName->type == NM_ENUM)
		{
			outCode2(mov_eax, pName->address);
			setValue(VAL, 0, ID.INT, pv);
			pv->ival = pName->address;
			pv->fConst = TRUE;
			return;
		}
		type = pName->dataType;
		ptrs = pName->ptrs + getPtr(type);
		if (pName->addrType == AD_CODE)
		{
			loadAddr(AD_CODE, hid);
			setValue(VAL, ptrs + 1, type, pv);
			return;
		}
		loadAddr(pName->addrType, pName->addrType == AD_IMPORT ? hid : pName->address);
	}
	depth = 0;
	while ((fArray = ispp('[')) || (fDot = ispp('.')) || (fArrow = is2pp("->")))
	{
		if (fArray)
		{
			ptrs--;
			size = pName->size[++depth];
			if (size == 0) size = ptrs > 0 ? 4 : sizeOfDataType(pName->dataType);
			if (pName->size[depth] == 0)
			{
				int *pI = &cd.pCode[ix.ixCode - 1].inst;
				if (*pI == lea_eax_pbp) *pI = mov_eax_pbp;
				else outCode1(mov_eax_pax);
			}
			int ixAddr = ix.ixCode - 1;
			outCode1(push_eax);
			int bgn = ix.ixCode;
			expression(VAL, pv);
			skip(']');
			if (pv->fConst)
			{
				ix.ixCode = bgn - 1;
				outCode2(add_eax, pv->ival*size);
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
					if (size > 1) outCode2(imul_edx_edx, size);
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
					if (size > 1) outCode2(imul_eax_eax, size);
					outCode1(add_eax_edx);
				}
			}
		}
		else
		{
			int id = cd.token[ix.tix++].ival;
			pName = getAttr(type, id);
			if (pName == NULL)
				error("prim", "'%s' not struct member", toString(id));
			if (!fDone && fArrow) outCode1(mov_eax_pax);
			fDone = 0;
			fArrow = 0;
			ptrs = pName->ptrs;
			type = pName->dataType;
			depth = 0;
			outCode2(add_eax, pName->address);
		}
	}
	if (((fInc = is2pp("++")) || is2pp("--")) && mode == VAL)
	{
		outCode1(mov_edx_eax);
		loadValue(type, ptrs > 0);
		incdec(type, ptrs > 0, fInc, 'd');
		int k = ix.ixCode - 1;
		int inst = cd.pCode[k].inst;
		if (cd.pCode[k - 3].inst == lea_eax_pbp && cd.pCode[k - 2].inst == mov_edx_eax
			&& cd.pCode[k - 1].inst == mov_eax_pax && cd.pCode[k].num == 1
			&& (inst == add_ddx || inst == sub_ddx))
		{
			cd.pCode[k - 3].inst = mov_eax_pbp;
			cd.pCode[k - 2] = cd.pCode[k - 3];
			cd.pCode[k - 2].inst = inst == add_ddx ? inc_dbp : dec_dbp;
			ix.ixCode -= 2;
		}
	}
	else if (pName->size[depth + 1] == 0 && mode == VAL)
	{
		loadValue(type, ptrs > 0);
	}
	setValue(mode, ptrs, type, pv);
}

void primaryExpression(int mode, VALUE *pv)
{
	if (cd.token[ix.tix].type >= TK_STRING)
	{
		constant(pv);
	}
	else if (is(ID.SIZEOF))
	{
		sizeOf(pv);
	}
	else if (!is('(') && isN('(', 1))
	{
		functionCall(pv);
		if (pv->type == ID.DOUBLE)
			outCode1(fstp_st1);
	}
	else
	{
		primExpr(mode, pv);
	}
}

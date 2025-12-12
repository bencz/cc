/*
 * codegen.c - Geração de Código Intermediário
 * 
 * Este módulo contém funções para emissão de código intermediário
 * que será posteriormente traduzido pelo backend específico (x86, HLASM, etc).
 */

#include "cc.h"

/*============================================================================
 * Gerenciamento de Buffer de Código
 *============================================================================*/

void reallocCode(int size)
{
	if (ix.ixCode + size + 10 < cd.sizeCode) return;
	cd.sizeCode = cd.sizeCode * 3 / 2;
	cd.pCode = xrealloc(cd.pCode, cd.sizeCode*sizeof(INSTRUCT));
}

/*============================================================================
 * Emissão de Instruções
 *============================================================================*/

void outCode3(int code, int num, int attr)
{
	if (code <= 0) error("outCode3", "code = %d", code);
	reallocCode(1);
	INSTRUCT inst = { code, num, attr };
	memcpy(&cd.pCode[ix.ixCode], &inst, sizeof(inst));
	cd.pCode[ix.ixCode++].regs = instruction[code - 1].regs_size & 0xFF00;
}

void outCode2(int code, int num)
{
	outCode3(code, num, 0);
}

void outCode1(int code)
{
	outCode3(code, 0, 0);
}

/*============================================================================
 * Manipulação de Código
 *============================================================================*/

void delCodes(int from, int to)
{
	memmove(&cd.pCode[from], &cd.pCode[to], (ix.ixCode - to)*sizeof(INSTRUCT));
	ix.ixCode -= (to - from);
}

void delCode(int n)
{
	delCodes(n, n + 1);
}

/*============================================================================
 * Controle de Fluxo
 *============================================================================*/

void jumpFalse(int loc, char *msg)
{
	int in = cd.pCode[ix.ixCode - 1].inst;
	if (in >= sete_eax && in <= setg_eax)
	{
		cd.pCode[ix.ixCode - 1].inst = in == setg_eax ? jle : in == setge_eax ? jl
			: in == setl_eax ? jge : in == setle_eax ? jg : in == sete_eax ? jnz : jz;
		cd.pCode[ix.ixCode - 1].num = loc;
	}
	else
	{
		outCode1(test_eax_eax);
		outCode2(jz, loc);
	}
}

void jumpTrue(int loc, char *msg)
{
	int in = cd.pCode[ix.ixCode - 1].inst;
	if (in >= sete_eax && in <= setg_eax)
	{
		cd.pCode[ix.ixCode - 1].inst = in == setg_eax ? jg : in == setge_eax ? jge
			: in == setl_eax ? jl : in == setle_eax ? jle : in == sete_eax ? jz : jnz;
		cd.pCode[ix.ixCode - 1].num = loc;
	}
	else
	{
		outCode1(test_eax_eax);
		outCode2(jnz, loc);
	}
}

int loc(void)
{
	return ++ix.ixLoc;
}

/*============================================================================
 * Emissão de Dados
 *============================================================================*/

void outData(int inst, int offset)
{
	reallocCode(1);
	cd.pCode[ix.ixCode].inst = inst;
	cd.pCode[ix.ixCode].offset = offset;
}

void outInteger(int offset, int size, int ival)
{
	outData(setint, offset);
	cd.pCode[ix.ixCode].attr = size;
	cd.pCode[ix.ixCode++].num = ival;
}

void outReal(int offset, double dval)
{
	outData(setreal, offset);
	cd.pCode[ix.ixCode++].dval = dval;
}

int outString(int offset, char *sval)
{
	outData(setstr, offset);
	cd.pCode[ix.ixCode++].sval = sval;
	return xstrlen(sval);
}

void outDataChar(int c)
{
	outInteger(ix.ixData, 1, c);
	ix.ixData++;
}

void outDataShort(int n)
{
	outInteger(ix.ixData, 2, n);
	ix.ixData += 2;
}

void outDataInt(int n)
{
	outInteger(ix.ixData, 4, n);
	ix.ixData += 4;
}

void outDataAddr(int p)
{
	outInteger(ix.ixData, 0, p);
	ix.ixData += 4;
}

void outDataDouble(double d)
{
	outReal(ix.ixData, d);
	ix.ixData += 8;
}

/*============================================================================
 * Load/Store
 *============================================================================*/

void loadAddr(int type, int addr)
{
	if (type == AD_STACK) outCode2(lea_eax_pbp, addr);
	else outCode3(mov_eax, addr, type);
}

void loadValue(int type, int fPtr)
{
	int *pI = &cd.pCode[ix.ixCode - 1].inst;
	if (type == ID.INT || fPtr)
	{
		if (*pI >= lea_eax_ad1 && *pI <= lea_eax_ad8)
			*pI = mov_eax_ad1 + (*pI - lea_eax_ad1);
		else if (*pI == lea_eax_pbp) *pI = mov_eax_pbp;
		else outCode1(mov_eax_pax);
	}
	else if (type == ID.CHAR)
	{
		outCode1(movsx_eax_bax);
	}
	else if (type == ID.SHORT)
	{
		outCode1(movsx_eax_wax);
	}
	else if (type == ID.DOUBLE)
	{
		if (*pI == lea_eax_pbp) *pI = fld_qbp;
		else if (*pI == mov_eax) *pI = fld_qp;
		else outCode1(fld_qax);
	}
	else
	{
		error("code.loadValue", "undefined type '%s'", toString(type));
	}
}

void setFpuStack2(int type1, int type2)
{
	if (type1 == ID.INT)
	{
		outCode2(fild_dsp, 0);
		outCode2(add_esp, 4);
		outCode1(fxch_st1);
	}
	else if (type2 == ID.INT)
	{
		outCode2(mov_psp_eax, -4);
		outCode2(fild_dsp, -4);
	}
}

void incdec(int type, int fPtr, int fI, int reg)
{
	if (type == ID.INT || fPtr) outCode2(reg == 'a' ? (fI ? add_dax : sub_dax) : (fI ? add_ddx : sub_ddx), 1);
	else if (type == ID.CHAR) outCode2(reg == 'a' ? (fI ? add_bax : sub_bax) : (fI ? add_bdx : sub_bdx), 1);
	else error("expr.incdec", "wrong use of ++ or --", toString(type));
}

/*============================================================================
 * Funções Auxiliares de Expressão
 *============================================================================*/

void expr(int mode)
{
	VALUE v;
	expression(mode, &v);
}

void expr2(int mode)
{
	skip('(');
	expr(mode);
	skip(')');
}

void setValue(int mode, int ptrs, int type, VALUE *pv)
{
	VALUE val = { mode, ptrs, type };
	memcpy(pv, &val, sizeof(VALUE));
}

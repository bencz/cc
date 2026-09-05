/* Target-independent IR emission helpers. */

#include "cc.h"

/* Code buffer */

void reallocCode(int size)
{
	size_t required;
	size_t capacity;
	size_t oldCapacity;

	if (size < 0 || ix.ixCode < 0)
	{
		error("reallocCode", "invalid code-buffer request");
	}
	required = (size_t)ix.ixCode + (size_t)size + 10U;
	if (required <= (size_t)cd.sizeCode)
	{
		return;
	}

	oldCapacity = cd.sizeCode > 0 ? (size_t)cd.sizeCode : 0U;
	capacity = oldCapacity > 0U ? oldCapacity : 256U;
	while (capacity < required)
	{
		if (capacity > (SIZE_MAX - 1U) / 3U * 2U)
		{
			error("reallocCode", "code buffer is too large");
		}
		capacity = capacity + capacity / 2U + 1U;
	}
	if (capacity > (size_t)INT_MAX || capacity > SIZE_MAX / sizeof(INSTRUCT))
	{
		error("reallocCode", "code buffer is too large");
	}
	cd.pCode = xrealloc(cd.pCode, capacity * sizeof(INSTRUCT));
	memset(cd.pCode + oldCapacity, 0, (capacity - oldCapacity) * sizeof(INSTRUCT));
	cd.sizeCode = (int)capacity;
}

/* Instruction emission */

void outCode3(int code, int num, int attr)
{
	if (code <= 0)
	{
		error("outCode3", "code = %d", code);
	}
	reallocCode(1);
	INSTRUCT inst = {0};
	inst.inst = code;
	inst.num = num;
	inst.attr = attr;
	memcpy(&cd.pCode[ix.ixCode], &inst, sizeof(inst));
	cd.pCode[ix.ixCode++].regs = instructionRegisters(code);
}

void outCode2(int code, int num)
{
	outCode3(code, num, 0);
}

void outCode1(int code)
{
	outCode3(code, 0, 0);
}

/* Instruction removal */

void delCodes(int from, int to)
{
	memmove(&cd.pCode[from], &cd.pCode[to], (ix.ixCode - to) * sizeof(INSTRUCT));
	ix.ixCode -= (to - from);
}

void delCode(int n)
{
	delCodes(n, n + 1);
}

/* Control flow */

static void emitConditionalJump(int location, int jumpWhenTrue)
{
	static const int trueBranches[] = {jz, jnz, jl, jge, jle, jg};
	static const int falseBranches[] = {jnz, jz, jge, jl, jg, jle};
	int instructionCode = cd.pCode[ix.ixCode - 1].inst;

	if (instructionCode >= sete_eax && instructionCode <= setg_eax)
	{
		int comparison = instructionCode - sete_eax;
		cd.pCode[ix.ixCode - 1].inst =
		    jumpWhenTrue ? trueBranches[comparison] : falseBranches[comparison];
		cd.pCode[ix.ixCode - 1].num = location;
	}
	else
	{
		outCode1(test_eax_eax);
		outCode2(jumpWhenTrue ? jnz : jz, location);
	}
}

void jumpFalse(int location)
{
	emitConditionalJump(location, 0);
}

void jumpTrue(int location)
{
	emitConditionalJump(location, 1);
}

int loc(void)
{
	return ++ix.ixLoc;
}

/* Static data */

static void outData(int inst, int offset)
{
	reallocCode(1);
	cd.pCode[ix.ixCode].inst = inst;
	cd.pCode[ix.ixCode].offset = offset;
}

static void outInteger(int offset, int size, int ival)
{
	outData(setint, offset);
	cd.pCode[ix.ixCode].attr = size;
	cd.pCode[ix.ixCode++].num = ival;
}

static void outReal(int offset, double dval)
{
	outData(setreal, offset);
	cd.pCode[ix.ixCode++].dval = dval;
}

int outString(int offset, char *sval)
{
	outData(setstr, offset);
	cd.pCode[ix.ixCode++].sval = sval;
	return decodedStringLength(sval);
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

/* Loads and stores */

void loadAddr(int type, int addr)
{
	if (type == AD_STACK)
	{
		outCode2(lea_eax_pbp, addr);
	}
	else
	{
		outCode3(mov_eax, addr, type);
	}
}

void loadValue(int type, int fPtr)
{
	int *pI = &cd.pCode[ix.ixCode - 1].inst;
	int canonical = canonicalType(type);
	int objectSize = sizeOfDataType(type);
	int smallAggregate = !fPtr && !isIntegerType(type) && canonical != ID.T_VOID &&
	                     canonical != ID.T_FLOAT && canonical != ID.T_DOUBLE &&
	                     objectSize <= (int)sizeof(int);
	type = canonical;

	if ((isIntegerType(type) && objectSize == 4) || fPtr || (smallAggregate && objectSize == 4))
	{
		if (*pI >= lea_eax_ad1 && *pI <= lea_eax_ad8)
		{
			*pI = mov_eax_ad1 + (*pI - lea_eax_ad1);
		}
		else if (*pI == lea_eax_pbp)
		{
			*pI = mov_eax_pbp;
		}
		else
		{
			outCode1(mov_eax_pax);
		}
	}
	else if (smallAggregate && objectSize == 2)
	{
		outCode1(movzx_eax_wax);
	}
	else if (smallAggregate && objectSize == 1)
	{
		outCode1(movzx_eax_bax);
	}
	else if (type == ID.T_BOOL || type == ID.T_UCHAR)
	{
		outCode1(movzx_eax_bax);
	}
	else if (type == ID.T_CHAR || type == ID.T_SCHAR)
	{
		outCode1(movsx_eax_bax);
	}
	else if (type == ID.T_USHORT)
	{
		outCode1(movzx_eax_wax);
	}
	else if (type == ID.T_SHORT)
	{
		outCode1(movsx_eax_wax);
	}
	else if (isFloatingType(type))
	{
		if (*pI == lea_eax_pbp)
		{
			*pI = fld_qbp;
		}
		else if (*pI == mov_eax)
		{
			*pI = fld_qp;
		}
		else
		{
			outCode1(fld_qax);
		}
	}
	else
	{
		error("code.loadValue", "undefined type '%s'", toString(type));
	}
}

void setFpuStack2(int type1, int type2)
{
	if (isIntegerType(type1))
	{
		outCode1(isUnsignedType(integerPromotion(type1)) ? fild_udsp : fild_dsp);
		outCode2(add_esp, 4);
		outCode1(fxch_st1);
	}
	else if (isIntegerType(type2))
	{
		if (isUnsignedType(integerPromotion(type2)))
		{
			outCode1(fild_uax);
		}
		else
		{
			outCode2(mov_psp_eax, -4);
			outCode2(fild_dsp, -4);
		}
	}
}

void incdec(int type, int ptrs, int fIncrement, int reg)
{
	int width = sizeOfObjectType(type, ptrs);
	int amount = ptrs > 0 ? sizeOfObjectType(type, ptrs - 1) : 1;
	if (width == 4)
	{
		outCode2(reg == 'a' ? (fIncrement ? add_dax : sub_dax) : (fIncrement ? add_ddx : sub_ddx),
		         amount);
	}
	else if (width == 2)
	{
		outCode2(reg == 'a' ? (fIncrement ? add_wax : sub_wax) : (fIncrement ? add_wdx : sub_wdx),
		         amount);
	}
	else if (width == 1)
	{
		outCode2(reg == 'a' ? (fIncrement ? add_bax : sub_bax) : (fIncrement ? add_bdx : sub_bdx),
		         amount);
	}
	else
	{
		error("expr.incdec", "wrong use of ++ or --", toString(type));
	}
}

/* Expression helpers */

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
	VALUE val = {0};
	val.mode = mode;
	val.ptrs = ptrs;
	val.type = type;
	val.irValue = IR_VALUE_NONE;
	val.irAddress = IR_VALUE_NONE;
	val.constantSymbol = IR_SYMBOL_NONE;
	memcpy(pv, &val, sizeof(VALUE));
}

/* Lower the typed CFG IR to the private IA-32 machine-instruction stream. */

#include "cc.h"

typedef struct _X86_IR_FUNCTION
{
	const IrFunction *function;
	int *localOffsets;
	int *valueOffsets;
	int *blockLocations;
	int frameSize;
	int returnBytes;
} X86_IR_FUNCTION;

typedef struct _X86_IR_GLOBAL
{
	IrSymbolId symbol;
	int address;
} X86_IR_GLOBAL;

typedef struct _X86_IR_FUNCTION_SYMBOL
{
	IrSymbolId symbol;
	int linkerSymbol;
} X86_IR_FUNCTION_SYMBOL;

typedef struct _X86_IR_STATE
{
	X86_IR_FUNCTION function;
	X86_IR_GLOBAL *globals;
	int globalCount;
	X86_IR_FUNCTION_SYMBOL *functionSymbols;
	int functionSymbolCount;
} X86_IR_STATE;

static X86_IR_STATE xs;

static int parameterOffset(const IrFunction *function, int parameterIndex);

static int alignUp(int value, int alignment)
{
	int remainder;
	if (alignment <= 0)
	{
		error("x86 IR", "invalid alignment %d", alignment);
	}
	remainder = value % alignment;
	return remainder == 0 ? value : value + alignment - remainder;
}

static int typeSize(IrType type)
{
	if (type.kind == IR_TYPE_VOID)
	{
		return 0;
	}
	if (type.bits == 0U || (type.bits & 7U) != 0U)
	{
		error("x86 IR", "invalid type width %u", type.bits);
	}
	return type.bits / CHAR_BIT;
}

static IrType valueType(IrValueId value)
{
	const IrFunction *function = xs.function.function;
	int blockIndex;
	if (value < 0 || value >= function->nextValue)
	{
		error("x86 IR", "invalid value %d", value);
	}
	for (blockIndex = 0; blockIndex < function->blockCount; ++blockIndex)
	{
		const IrBasicBlock *block = &function->blocks[blockIndex];
		int instructionIndex;
		for (instructionIndex = 0; instructionIndex < block->instructionCount; ++instructionIndex)
		{
			const IrInstruction *instruction = &block->instructions[instructionIndex];
			if (instruction->result == value)
			{
				return instruction->type;
			}
		}
	}
	error("x86 IR", "value %d has no definition", value);
}

static void loadInteger(IrValueId value)
{
	outCode3(mov_eax_pbp, xs.function.valueOffsets[value], AD_STACK);
}

static void storeInteger(IrValueId value)
{
	outCode3(mov_pbp_eax, xs.function.valueOffsets[value], AD_STACK);
}

static void loadFloat(IrValueId value)
{
	IrType type = valueType(value);
	if (type.kind != IR_TYPE_FLOAT || type.bits != 64U)
	{
		error("x86 IR", "only IEEE binary64 values are supported");
	}
	outCode3(fld_qbp, xs.function.valueOffsets[value], AD_STACK);
}

static void storeFloat(IrValueId value)
{
	outCode3(fstp_qbp, xs.function.valueOffsets[value], AD_STACK);
}

static void loadIndirect(IrType type)
{
	if (type.kind == IR_TYPE_FLOAT)
	{
		if (type.bits != 64U)
		{
			error("x86 IR", "only IEEE binary64 loads are supported");
		}
		outCode1(fld_qax);
	}
	else if (type.bits == 32U)
	{
		outCode1(mov_eax_pax);
	}
	else if (type.bits == 16U)
	{
		outCode1(type.isUnsigned ? movzx_eax_wax : movsx_eax_wax);
	}
	else if (type.bits == 8U)
	{
		outCode1(type.isUnsigned ? movzx_eax_bax : movsx_eax_bax);
	}
	else
	{
		error("x86 IR", "unsupported load width %u", type.bits);
	}
}

static int globalAddress(IrSymbolId symbol)
{
	int index;
	for (index = 0; index < xs.globalCount; ++index)
	{
		if (xs.globals[index].symbol == symbol)
		{
			return xs.globals[index].address;
		}
	}
	error("x86 IR", "unknown global %d", symbol);
}

static int functionLinkerSymbol(IrSymbolId symbol)
{
	int index;

	if (symbol >= 0)
	{
		return symbol;
	}
	for (index = 0; index < xs.functionSymbolCount; ++index)
	{
		if (xs.functionSymbols[index].symbol == symbol)
		{
			return xs.functionSymbols[index].linkerSymbol;
		}
	}
	error("x86 IR", "unknown function symbol %d", symbol);
}

static void loadSymbolAddress(IrSymbolId symbol, int offset)
{
	if (symbol < 0)
	{
		IrGlobal *global = irFindGlobal(&compiler.ir, symbol);

		if (global != NULL && !global->isExternal)
		{
			outCode3(mov_eax, globalAddress(symbol) + offset, AD_DATA);
		}
		else
		{
			outCode3(mov_eax, functionLinkerSymbol(symbol), AD_CODE);
			if (offset != 0)
			{
				outCode2(add_eax, offset);
			}
		}
	}
	else
	{
		Name *name = getNameFromTable(globTable, NM_VAR, symbol);
		if (name != NULL)
		{
			outCode3(mov_eax,
			         (name->addrType == AD_IMPORT ? symbol : name->address) + offset,
			         name->addrType);
		}
		else
		{
			outCode3(mov_eax, symbol, AD_CODE);
			if (offset != 0)
			{
				outCode2(add_eax, offset);
			}
		}
	}
}

static void emitComparison(const IrInstruction *instruction)
{
	IrType operandType = valueType(instruction->left);
	if (operandType.kind == IR_TYPE_FLOAT)
	{
		loadFloat(instruction->left);
		loadFloat(instruction->right);
		if (instruction->condition == IR_COMPARE_GREATER_SIGNED ||
		    instruction->condition == IR_COMPARE_GREATER_UNSIGNED ||
		    instruction->condition == IR_COMPARE_GREATER_EQUAL_SIGNED ||
		    instruction->condition == IR_COMPARE_GREATER_EQUAL_UNSIGNED)
		{
			outCode1(fxch_st1);
		}
		outCode1(fucompp);
		outCode1(fstsw);
		if (instruction->condition == IR_COMPARE_EQUAL ||
		    instruction->condition == IR_COMPARE_NOT_EQUAL)
		{
			outCode2(and_ah, 0x45);
			outCode2(instruction->condition == IR_COMPARE_EQUAL ? cmp_ah : xor_ah, 0x40);
			outCode1(instruction->condition == IR_COMPARE_EQUAL ? sete_eax : setne_eax);
		}
		else
		{
			int strict = instruction->condition == IR_COMPARE_LESS_SIGNED ||
			             instruction->condition == IR_COMPARE_LESS_UNSIGNED ||
			             instruction->condition == IR_COMPARE_GREATER_SIGNED ||
			             instruction->condition == IR_COMPARE_GREATER_UNSIGNED;
			outCode2(test_ah, strict ? 0x45 : 0x05);
			outCode1(sete_eax);
		}
	}
	else
	{
		int unsignedComparison = instruction->condition == IR_COMPARE_LESS_UNSIGNED ||
		                         instruction->condition == IR_COMPARE_LESS_EQUAL_UNSIGNED ||
		                         instruction->condition == IR_COMPARE_GREATER_UNSIGNED ||
		                         instruction->condition == IR_COMPARE_GREATER_EQUAL_UNSIGNED;
		loadInteger(instruction->left);
		outCode1(push_eax);
		loadInteger(instruction->right);
		outCode1(pop_ecx);
		outCode1(unsignedComparison ? ucmp_ecx_eax : cmp_ecx_eax);
		switch (instruction->condition)
		{
		case IR_COMPARE_EQUAL:
			outCode1(sete_eax);
			break;
		case IR_COMPARE_NOT_EQUAL:
			outCode1(setne_eax);
			break;
		case IR_COMPARE_LESS_SIGNED:
			outCode1(setl_eax);
			break;
		case IR_COMPARE_LESS_UNSIGNED:
			outCode1(setb_eax);
			break;
		case IR_COMPARE_LESS_EQUAL_SIGNED:
			outCode1(setle_eax);
			break;
		case IR_COMPARE_LESS_EQUAL_UNSIGNED:
			outCode1(setbe_eax);
			break;
		case IR_COMPARE_GREATER_SIGNED:
			outCode1(setg_eax);
			break;
		case IR_COMPARE_GREATER_UNSIGNED:
			outCode1(seta_eax);
			break;
		case IR_COMPARE_GREATER_EQUAL_SIGNED:
			outCode1(setge_eax);
			break;
		case IR_COMPARE_GREATER_EQUAL_UNSIGNED:
			outCode1(setae_eax);
			break;
		default:
			error("x86 IR", "invalid comparison condition %d", instruction->condition);
		}
	}
	storeInteger(instruction->result);
}

static void emitBinary(const IrInstruction *instruction)
{
	IrOpcode opcode = instruction->opcode;
	if (instruction->type.kind == IR_TYPE_FLOAT)
	{
		loadFloat(instruction->left);
		loadFloat(instruction->right);
		outCode1(opcode == IR_OP_ADD             ? faddp_st1_st
		         : opcode == IR_OP_SUBTRACT      ? fsubrp_st1_st
		         : opcode == IR_OP_MULTIPLY      ? fmulp_st1_st
		         : opcode == IR_OP_DIVIDE_SIGNED ? fdivrp_st1_st
		                                         : 0);
		storeFloat(instruction->result);
		return;
	}
	loadInteger(instruction->left);
	outCode1(push_eax);
	loadInteger(instruction->right);
	if ((opcode == IR_OP_POINTER_ADD || opcode == IR_OP_POINTER_SUBTRACT) &&
	    instruction->offset != 1)
	{
		outCode2(imul_eax_eax, instruction->offset);
	}
	outCode1(pop_ecx);
	switch (opcode)
	{
	case IR_OP_ADD:
	case IR_OP_POINTER_ADD:
		outCode1(add_eax_ecx);
		break;
	case IR_OP_SUBTRACT:
	case IR_OP_POINTER_SUBTRACT:
		outCode1(xchg_eax_ecx);
		outCode1(sub_eax_ecx);
		break;
	case IR_OP_MULTIPLY:
		outCode1(imul_eax_ecx);
		break;
	case IR_OP_DIVIDE_SIGNED:
	case IR_OP_REMAINDER_SIGNED:
		outCode1(xchg_eax_ecx);
		outCode1(opcode == IR_OP_DIVIDE_SIGNED ? xdiv_ecx : xmod_ecx);
		break;
	case IR_OP_DIVIDE_UNSIGNED:
	case IR_OP_REMAINDER_UNSIGNED:
		outCode1(xchg_eax_ecx);
		outCode1(opcode == IR_OP_DIVIDE_UNSIGNED ? udiv_ecx : umod_ecx);
		break;
	case IR_OP_BITWISE_AND:
		outCode1(and_eax_ecx);
		break;
	case IR_OP_BITWISE_OR:
		outCode1(or_eax_ecx);
		break;
	case IR_OP_BITWISE_XOR:
		outCode1(xor_eax_ecx);
		break;
	case IR_OP_SHIFT_LEFT:
		outCode1(xchg_eax_ecx);
		outCode1(shl_eax_cl);
		break;
	case IR_OP_SHIFT_RIGHT_SIGNED:
		outCode1(xchg_eax_ecx);
		outCode1(sar_eax_cl);
		break;
	case IR_OP_SHIFT_RIGHT_UNSIGNED:
		outCode1(xchg_eax_ecx);
		outCode1(shr_eax_cl);
		break;
	case IR_OP_POINTER_DIFFERENCE:
		outCode1(xchg_eax_ecx);
		outCode1(sub_eax_ecx);
		outCode2(mov_ecx, instruction->offset);
		outCode1(xdiv_ecx);
		break;
	default:
		error("x86 IR", "invalid binary opcode %d", opcode);
	}
	storeInteger(instruction->result);
}

static void emitConversion(const IrInstruction *instruction)
{
	IrType source = valueType(instruction->left);
	IrType target = instruction->type;
	if (source.kind == IR_TYPE_FLOAT && target.kind == IR_TYPE_FLOAT)
	{
		loadFloat(instruction->left);
		storeFloat(instruction->result);
	}
	else if (source.kind == IR_TYPE_FLOAT)
	{
		loadFloat(instruction->left);
		outCode3(fldcw, 2, AD_DATA);
		if (target.isUnsigned)
		{
			outCode1(fistp_ueax);
		}
		else
		{
			outCode2(fistp_dsp, -4);
			outCode2(mov_eax_psp, -4);
		}
		outCode3(fldcw, 0, AD_DATA);
		if (target.bits < 32U)
		{
			int shift = target.bits == 8U ? 24 : 16;
			outCode2(shl_eax, shift);
			outCode2(target.isUnsigned ? shr_eax : sar_eax, shift);
		}
		storeInteger(instruction->result);
	}
	else if (target.kind == IR_TYPE_FLOAT)
	{
		loadInteger(instruction->left);
		if (source.isUnsigned)
		{
			outCode1(fild_uax);
		}
		else
		{
			outCode2(mov_psp_eax, -4);
			outCode2(fild_dsp, -4);
		}
		storeFloat(instruction->result);
	}
	else
	{
		loadInteger(instruction->left);
		if (target.bits < 32U)
		{
			int shift = target.bits == 8U ? 24 : 16;
			outCode2(shl_eax, shift);
			outCode2(target.isUnsigned ? shr_eax : sar_eax, shift);
		}
		storeInteger(instruction->result);
	}
}

static int parameterOffset(const IrFunction *function, int parameterIndex)
{
	int index;
	int offset = 8;
	for (index = 0; index < parameterIndex; ++index)
	{
		offset += alignUp(typeSize(function->parameterTypes[index]), 4);
	}
	return offset;
}

static void emitCall(const IrInstruction *instruction)
{
	int argumentIndex;
	int argumentBytes = 0;
	for (argumentIndex = instruction->argumentCount; argumentIndex-- > 0;)
	{
		IrValueId argument = instruction->arguments[argumentIndex];
		IrType type = valueType(argument);
		if (type.kind == IR_TYPE_FLOAT)
		{
			loadFloat(argument);
			outCode2(sub_esp, 8);
			outCode1(fstp_qsp);
			argumentBytes += 8;
		}
		else
		{
			loadInteger(argument);
			outCode1(push_eax);
			argumentBytes += 4;
		}
	}
	if (instruction->opcode == IR_OP_CALL_INDIRECT)
	{
		loadInteger(instruction->left);
		outCode1(call_eax);
	}
	else
	{
		outCode2(call, functionLinkerSymbol(instruction->symbol));
	}
	if (instruction->callingConvention == IR_CALL_C && argumentBytes > 0)
	{
		outCode2(add_esp, argumentBytes);
	}
	if (instruction->result != IR_VALUE_NONE)
	{
		if (instruction->type.kind == IR_TYPE_FLOAT)
		{
			storeFloat(instruction->result);
		}
		else
		{
			storeInteger(instruction->result);
		}
	}
}

static void emitInstruction(const IrInstruction *instruction)
{
	switch (instruction->opcode)
	{
	case IR_OP_PARAMETER:
		if (instruction->type.kind == IR_TYPE_FLOAT)
		{
			outCode3(fld_qbp, parameterOffset(xs.function.function, instruction->offset), AD_STACK);
			storeFloat(instruction->result);
		}
		else
		{
			outCode3(
			    mov_eax_pbp, parameterOffset(xs.function.function, instruction->offset), AD_STACK);
			storeInteger(instruction->result);
		}
		break;
	case IR_OP_UNDEFINED:
		break;
	case IR_OP_CONSTANT_INTEGER:
		outCode2(mov_eax, (int)(uint32_t)instruction->integer);
		storeInteger(instruction->result);
		break;
	case IR_OP_CONSTANT_FLOAT:
		outDataDouble(instruction->floating);
		outCode3(fld_qp, ix.ixData - 8, AD_DATA);
		storeFloat(instruction->result);
		break;
	case IR_OP_ADDRESS_OF:
		loadSymbolAddress(instruction->symbol, instruction->offset);
		storeInteger(instruction->result);
		break;
	case IR_OP_LOCAL_ADDRESS:
		outCode3(lea_eax_pbp,
		         xs.function.localOffsets[instruction->local] + instruction->offset,
		         AD_STACK);
		storeInteger(instruction->result);
		break;
	case IR_OP_LOAD:
		loadInteger(instruction->left);
		loadIndirect(instruction->type);
		if (instruction->type.kind == IR_TYPE_FLOAT)
		{
			storeFloat(instruction->result);
		}
		else
		{
			storeInteger(instruction->result);
		}
		break;
	case IR_OP_STORE:
		loadInteger(instruction->left);
		outCode1(mov_ecx_eax);
		if (instruction->type.kind == IR_TYPE_FLOAT)
		{
			loadFloat(instruction->right);
			outCode1(fst_qcx);
			outCode1(fstp_st1);
		}
		else
		{
			loadInteger(instruction->right);
			outCode1(instruction->type.bits == 32U   ? mov_pcx_eax
			         : instruction->type.bits == 16U ? mov_pcx_ax
			                                         : mov_pcx_al);
		}
		break;
	case IR_OP_ZERO_MEMORY:
	{
		int byteIndex;
		loadInteger(instruction->left);
		for (byteIndex = 0; byteIndex < instruction->offset; ++byteIndex)
		{
			outCode2(mov_bax, 0);
			if (byteIndex + 1 < instruction->offset)
			{
				outCode2(add_eax, 1);
			}
		}
		break;
	}
	case IR_OP_COPY:
		if (instruction->type.kind == IR_TYPE_FLOAT)
		{
			loadFloat(instruction->left);
			storeFloat(instruction->result);
		}
		else
		{
			loadInteger(instruction->left);
			storeInteger(instruction->result);
		}
		break;
	case IR_OP_ADD:
	case IR_OP_SUBTRACT:
	case IR_OP_MULTIPLY:
	case IR_OP_DIVIDE_SIGNED:
	case IR_OP_DIVIDE_UNSIGNED:
	case IR_OP_REMAINDER_SIGNED:
	case IR_OP_REMAINDER_UNSIGNED:
	case IR_OP_BITWISE_AND:
	case IR_OP_BITWISE_OR:
	case IR_OP_BITWISE_XOR:
	case IR_OP_SHIFT_LEFT:
	case IR_OP_SHIFT_RIGHT_SIGNED:
	case IR_OP_SHIFT_RIGHT_UNSIGNED:
	case IR_OP_POINTER_ADD:
	case IR_OP_POINTER_SUBTRACT:
	case IR_OP_POINTER_DIFFERENCE:
		emitBinary(instruction);
		break;
	case IR_OP_NEGATE:
		if (instruction->type.kind == IR_TYPE_FLOAT)
		{
			loadFloat(instruction->left);
			outCode1(fchs);
			storeFloat(instruction->result);
		}
		else
		{
			loadInteger(instruction->left);
			outCode1(neg_eax);
			storeInteger(instruction->result);
		}
		break;
	case IR_OP_BITWISE_NOT:
		loadInteger(instruction->left);
		outCode1(not_eax);
		storeInteger(instruction->result);
		break;
	case IR_OP_COMPARE:
		emitComparison(instruction);
		break;
	case IR_OP_CONVERT:
		emitConversion(instruction);
		break;
	case IR_OP_CALL:
	case IR_OP_CALL_INDIRECT:
		emitCall(instruction);
		break;
	case IR_OP_BRANCH:
		outCode2(jmp, xs.function.blockLocations[instruction->trueBlock]);
		break;
	case IR_OP_BRANCH_CONDITIONAL:
		loadInteger(instruction->left);
		outCode1(test_eax_eax);
		outCode2(jnz, xs.function.blockLocations[instruction->trueBlock]);
		outCode2(jmp, xs.function.blockLocations[instruction->falseBlock]);
		break;
	case IR_OP_RETURN:
		if (instruction->type.kind == IR_TYPE_FLOAT)
		{
			loadFloat(instruction->left);
		}
		else if (instruction->type.kind != IR_TYPE_VOID)
		{
			loadInteger(instruction->left);
		}
		outCode2(xret, xs.function.returnBytes);
		break;
	default:
		error("x86 IR", "opcode %d has no complete lowering", instruction->opcode);
	}
}

static void layoutFunction(const IrFunction *function)
{
	int offset = 0;
	int localIndex;
	int blockIndex;
	int parameterIndex;
	memset(&xs.function, 0, sizeof(xs.function));
	xs.function.function = function;
	xs.function.localOffsets = xalloc((size_t)function->localCount * sizeof(int));
	xs.function.valueOffsets = xalloc((size_t)function->nextValue * sizeof(int));
	xs.function.blockLocations = xalloc((size_t)function->blockCount * sizeof(int));
	for (localIndex = 0; localIndex < function->localCount; ++localIndex)
	{
		const IrLocal *local = &function->locals[localIndex];
		if (local->parameterIndex >= 0)
		{
			xs.function.localOffsets[local->id] = parameterOffset(function, local->parameterIndex);
			continue;
		}
		offset = alignUp(offset, local->alignment);
		offset += (int)local->size;
		xs.function.localOffsets[local->id] = -offset;
	}
	for (blockIndex = 0; blockIndex < function->blockCount; ++blockIndex)
	{
		const IrBasicBlock *block = &function->blocks[blockIndex];
		int instructionIndex;
		xs.function.blockLocations[block->id] = loc();
		for (instructionIndex = 0; instructionIndex < block->instructionCount; ++instructionIndex)
		{
			const IrInstruction *instruction = &block->instructions[instructionIndex];
			if (instruction->result != IR_VALUE_NONE)
			{
				int size =
				    instruction->type.kind == IR_TYPE_FLOAT ? typeSize(instruction->type) : 4;
				offset = alignUp(offset, size > 4 ? 8 : 4);
				offset += size;
				xs.function.valueOffsets[instruction->result] = -offset;
			}
		}
	}
	xs.function.frameSize = alignUp(offset, 4);
	for (parameterIndex = 0; parameterIndex < function->parameterCount; ++parameterIndex)
	{
		xs.function.returnBytes += alignUp(typeSize(function->parameterTypes[parameterIndex]), 4);
	}
}

static void freeFunctionLayout(void)
{
	free(xs.function.localOffsets);
	free(xs.function.valueOffsets);
	free(xs.function.blockLocations);
	memset(&xs.function, 0, sizeof(xs.function));
}

static void appendDataRelocation(int offset, int addressType, IrSymbolId symbol, int addend)
{
	INSTRUCT instruction = {setaddr, symbol, addressType};

	instruction.offset = offset;
	instruction.refs = addend;
	reallocCode(1);
	memcpy(&cd.pCode[ix.ixCode++], &instruction, sizeof(instruction));
}

static void describeRelocationTarget(IrSymbolId symbol, int *addressType, int *address)
{
	IrGlobal *global = irFindGlobal(&compiler.ir, symbol);
	Name *name;

	if (global != NULL && !global->isExternal)
	{
		*addressType = AD_DATA;
		*address = globalAddress(symbol);
		return;
	}
	name = symbol >= 0 ? getNameFromTable(globTable, NM_VAR, symbol) : NULL;
	if (name != NULL && name->addrType == AD_IMPORT)
	{
		*addressType = AD_IMPORT;
		*address = symbol;
		return;
	}
	*addressType = AD_CODE;
	*address = functionLinkerSymbol(symbol);
}

static void mapFunctionSymbols(void)
{
	int functionIndex;

	xs.functionSymbols = xalloc((size_t)compiler.ir.functionCount * sizeof(*xs.functionSymbols));
	for (functionIndex = 0; functionIndex < compiler.ir.functionCount; ++functionIndex)
	{
		IrSymbolId symbol = compiler.ir.functions[functionIndex].symbol;
		int linkerSymbol = symbol;

		if (symbol < 0)
		{
			char name[64];
			intptr_t attributes;

			snprintf(name, sizeof(name), "__cc_internal_%d", functionIndex);
			linkerSymbol = id(name);
			attributes = (intptr_t)cd.hash.tbl[linkerSymbol].val | AT_USER;
			cd.hash.tbl[linkerSymbol].val = (void *)attributes;
		}
		xs.functionSymbols[xs.functionSymbolCount].symbol = symbol;
		xs.functionSymbols[xs.functionSymbolCount].linkerSymbol = linkerSymbol;
		++xs.functionSymbolCount;
	}
}

static void layoutGlobals(void)
{
	int globalIndex;

	xs.globals = xalloc((size_t)compiler.ir.globalCount * sizeof(*xs.globals));
	for (globalIndex = 0; globalIndex < compiler.ir.globalCount; ++globalIndex)
	{
		const IrGlobal *global = &compiler.ir.globals[globalIndex];
		Name *name;

		if (global->isExternal)
		{
			continue;
		}
		ix.ixData = alignUp(ix.ixData, global->alignment);
		xs.globals[xs.globalCount].symbol = global->symbol;
		xs.globals[xs.globalCount].address = ix.ixData;
		++xs.globalCount;
		if (global->symbol >= 0)
		{
			name = getNameFromTable(globTable, NM_VAR, global->symbol);
			if (name == NULL)
			{
				error("x86 IR", "global symbol '%s' has no declaration", toString(global->symbol));
			}
			name->addrType = AD_DATA;
			name->address = ix.ixData;
		}
		ix.ixData += (int)global->zeroFillSize;
	}
}

static void emitGlobalData(void)
{
	int globalIndex;
	int totalSize = ix.ixData;

	for (globalIndex = 0; globalIndex < compiler.ir.globalCount; ++globalIndex)
	{
		const IrGlobal *global = &compiler.ir.globals[globalIndex];
		size_t byteIndex = 0U;
		int relocationIndex = 0;

		if (global->isExternal)
		{
			continue;
		}
		ix.ixData = globalAddress(global->symbol);
		while (byteIndex < global->zeroFillSize)
		{
			if (relocationIndex < global->relocationCount &&
			    global->relocations[relocationIndex].offset == byteIndex)
			{
				const IrRelocation *relocation = &global->relocations[relocationIndex];
				int addressType;
				int address;

				describeRelocationTarget(relocation->symbol, &addressType, &address);
				appendDataRelocation(ix.ixData, addressType, address, relocation->addend);
				ix.ixData += 4;
				byteIndex += 4U;
				++relocationIndex;
			}
			else
			{
				unsigned char byte =
				    byteIndex < global->initializerSize ? global->initializer[byteIndex] : 0U;

				if (byte != 0U)
				{
					outDataChar(byte);
				}
				else
				{
					++ix.ixData;
				}
				++byteIndex;
			}
		}
	}
	ix.ixData = totalSize;
}

void x86LowerIr(void)
{
	int functionIndex;

	ix.ixCode = 0;
	ix.ixData = 0;
	ix.ixZero = 0;
	ix.ixLoc = 0;
	mapFunctionSymbols();
	layoutGlobals();
	emitGlobalData();
	for (functionIndex = 0; functionIndex < compiler.ir.functionCount; ++functionIndex)
	{
		const IrFunction *function = &compiler.ir.functions[functionIndex];
		int blockIndex;
		layoutFunction(function);
		outCode2(function->isExported ? exp_ : fn_, functionLinkerSymbol(function->symbol));
		outCode1(xent);
		if (xs.function.frameSize > 0)
		{
			outCode2(sub_esp, xs.function.frameSize);
		}
		for (blockIndex = 0; blockIndex < function->blockCount; ++blockIndex)
		{
			const IrBasicBlock *block = &function->blocks[blockIndex];
			int instructionIndex;
			outCode2(loc_, xs.function.blockLocations[block->id]);
			for (instructionIndex = 0; instructionIndex < block->instructionCount;
			     ++instructionIndex)
			{
				emitInstruction(&block->instructions[instructionIndex]);
			}
		}
		freeFunctionLayout();
	}
	free(xs.globals);
	free(xs.functionSymbols);
	memset(&xs, 0, sizeof(xs));
}

/* Typed IR to PowerPC32. Values are spilled; ABI policy owns linkage decisions. */
#include "cc.h"
#include "ppc.h"

typedef struct _PpcFunctionState
{
	const IrFunction *function;
	IrType *types;
	int *values;
	int *locals;
	PpcArgumentLocation *parameters;
	int frameSize;
	int incomingRegisters;
	int scratch;
	int argumentCopies;
	int ordinal;
} PpcFunctionState;

typedef struct _PpcEmitter
{
	FILE *output;
	const PpcAbi *abi;
	PpcFunctionState frame;
	unsigned int serial;
} PpcEmitter;

static PpcEmitter power;

static int alignOffset(int value, int alignment)
{
	if (value < 0 || alignment <= 0 || (alignment & (alignment - 1)) != 0 ||
	    value > INT_MAX - alignment)
	{
		error("PowerPC", "stack or data layout exceeds the 32-bit target range");
	}
	return (value + alignment - 1) & -alignment;
}

static void symbolName(IrSymbolId symbol, char *name, size_t capacity)
{
	int count;
	if (symbol < IR_SYMBOL_NONE)
	{
		count = snprintf(name, capacity, "L..CCprivate%u", 0U - (unsigned int)symbol);
	}
	else
	{
		count = snprintf(name, capacity, "%s", toString(symbol));
	}
	if (count < 0 || (size_t)count >= capacity)
	{
		error("PowerPC", "symbol name exceeds assembler buffer");
	}
}

static void immediate(int reg, uint32_t value)
{
	int signedValue = (int32_t)value;
	if (signedValue >= -32768 && signedValue <= 32767)
	{
		fprintf(power.output, "\tli %d,%d\n", reg, signedValue);
	}
	else
	{
		fprintf(power.output,
		        "\tlis %d,%d\n\tori %d,%d,%u\n",
		        reg,
		        (int)(int16_t)(value >> 16),
		        reg,
		        reg,
		        value & 65535U);
	}
}

static void address(int reg, int base, int offset)
{
	if (offset >= -32768 && offset <= 32767)
	{
		fprintf(power.output, "\taddi %d,%d,%d\n", reg, base, offset);
	}
	else
	{
		/* r0 is never used as an address base. */
		immediate(0, (uint32_t)offset);
		fprintf(power.output, "\tadd %d,%d,0\n", reg, base);
	}
}

static void memory(const char *operation, int reg, int base, int offset)
{
	if (offset >= -32768 && offset <= 32767)
	{
		fprintf(power.output, "\t%s %d,%d(%d)\n", operation, reg, offset, base);
	}
	else
	{
		address(11, base, offset);
		fprintf(power.output, "\t%s %d,0(11)\n", operation, reg);
	}
}

static void loadInteger(IrValueId value, int reg)
{
	memory("lwz", reg, 1, power.frame.values[value]);
}

static void normalizeInteger(IrType type, int reg)
{
	if (type.bits == 8U)
	{
		if (type.isUnsigned)
		{
			fprintf(power.output, "\tclrlwi %d,%d,24\n", reg, reg);
		}
		else
		{
			fprintf(power.output, "\textsb %d,%d\n", reg, reg);
		}
	}
	else if (type.bits == 16U)
	{
		if (type.isUnsigned)
		{
			fprintf(power.output, "\tclrlwi %d,%d,16\n", reg, reg);
		}
		else
		{
			fprintf(power.output, "\textsh %d,%d\n", reg, reg);
		}
	}
}

static void storeInteger(IrValueId value, int reg)
{
	normalizeInteger(power.frame.types[value], reg);
	memory("stw", reg, 1, power.frame.values[value]);
}

static void loadFloat(IrValueId value, int reg)
{
	IrType type = power.frame.types[value];
	memory(type.bits == 32U ? "lfs" : "lfd", reg, 1, power.frame.values[value]);
}

static void storeFloat(IrValueId value, int reg)
{
	IrType type = power.frame.types[value];
	memory(type.bits == 32U ? "stfs" : "stfd", reg, 1, power.frame.values[value]);
}

static void floatingConstant(double value, int reg)
{
	unsigned char bytes[8];
	unsigned int endian = 1U;
	uint32_t high = 0U;
	uint32_t low = 0U;
	int index;
	memcpy(bytes, &value, sizeof(bytes));
	for (index = 0; index < 8; ++index)
	{
		unsigned int byte = bytes[*(unsigned char *)&endian ? 7 - index : index];
		if (index < 4)
		{
			high = (high << 8) | byte;
		}
		else
		{
			low = (low << 8) | byte;
		}
	}
	immediate(12, high);
	memory("stw", 12, 1, power.frame.scratch + 16);
	immediate(12, low);
	memory("stw", 12, 1, power.frame.scratch + 20);
	memory("lfd", reg, 1, power.frame.scratch + 16);
}

static void emitConversion(const IrInstruction *instruction)
{
	IrType source = power.frame.types[instruction->left];
	IrType target = instruction->type;
	if (source.kind == IR_TYPE_FLOAT && target.kind == IR_TYPE_FLOAT)
	{
		loadFloat(instruction->left, 0);
		storeFloat(instruction->result, 0);
	}
	else if (target.kind == IR_TYPE_FLOAT)
	{
		loadInteger(instruction->left, 3);
		if (!source.isUnsigned)
		{
			fputs("\txoris 3,3,32768\n", power.output);
		}
		immediate(12, 0x43300000U);
		memory("stw", 12, 1, power.frame.scratch);
		memory("stw", 3, 1, power.frame.scratch + 4);
		memory("lfd", 0, 1, power.frame.scratch);
		floatingConstant(source.isUnsigned ? 4503599627370496.0 : 4503601774854144.0, 1);
		fputs("\tfsub 0,0,1\n", power.output);
		storeFloat(instruction->result, 0);
	}
	else if (source.kind == IR_TYPE_FLOAT)
	{
		unsigned int label = power.serial++;
		loadFloat(instruction->left, 0);
		if (target.isUnsigned && target.bits == 32U)
		{
			floatingConstant(2147483648.0, 1);
			fputs("\tfcmpu 0,0,1\n", power.output);
			fprintf(power.output, "\tblt L..CCconvert%u\n\tfsub 0,0,1\n", label);
			fputs("\tfctiwz 0,0\n", power.output);
			memory("stfd", 0, 1, power.frame.scratch);
			memory("lwz", 3, 1, power.frame.scratch + 4);
			fprintf(power.output,
			        "\txoris 3,3,32768\n\tb L..CCconverted%u\nL..CCconvert%u:\n",
			        label,
			        label);
		}
		fputs("\tfctiwz 0,0\n", power.output);
		memory("stfd", 0, 1, power.frame.scratch);
		memory("lwz", 3, 1, power.frame.scratch + 4);
		fprintf(power.output, "L..CCconverted%u:\n", label);
		storeInteger(instruction->result, 3);
	}
	else
	{
		loadInteger(instruction->left, 3);
		storeInteger(instruction->result, 3);
	}
}

static void emitComparison(const IrInstruction *instruction)
{
	IrType type = power.frame.types[instruction->left];
	IrCompareCondition condition = instruction->condition;
	unsigned int label = power.serial++;
	const char *branch = "beq";
	if (type.kind == IR_TYPE_FLOAT)
	{
		loadFloat(instruction->left, 0);
		loadFloat(instruction->right, 1);
		fputs("\tfcmpu 0,0,1\n", power.output);
	}
	else
	{
		loadInteger(instruction->left, 3);
		loadInteger(instruction->right, 4);
		fprintf(power.output, "\t%s 0,3,4\n", type.isUnsigned ? "cmplw" : "cmpw");
	}
	if (condition == IR_COMPARE_NOT_EQUAL)
	{
		branch = "bne";
	}
	else if (condition == IR_COMPARE_LESS_SIGNED || condition == IR_COMPARE_LESS_UNSIGNED)
	{
		branch = "blt";
	}
	else if (condition == IR_COMPARE_GREATER_SIGNED || condition == IR_COMPARE_GREATER_UNSIGNED)
	{
		branch = "bgt";
	}
	else if (condition == IR_COMPARE_LESS_EQUAL_SIGNED ||
	         condition == IR_COMPARE_LESS_EQUAL_UNSIGNED)
	{
		fputs("\tcror 2,0,2\n", power.output);
	}
	else if (condition == IR_COMPARE_GREATER_EQUAL_SIGNED ||
	         condition == IR_COMPARE_GREATER_EQUAL_UNSIGNED)
	{
		fputs("\tcror 2,1,2\n", power.output);
	}
	fprintf(power.output,
	        "\tli 3,1\n\t%s L..CCcompare%u\n\tli 3,0\nL..CCcompare%u:\n",
	        branch,
	        label,
	        label);
	storeInteger(instruction->result, 3);
}

static void emitBinary(const IrInstruction *instruction)
{
	const char *operation = NULL;
	IrOpcode opcode = instruction->opcode;
	if (instruction->type.kind == IR_TYPE_FLOAT)
	{
		loadFloat(instruction->left, 0);
		loadFloat(instruction->right, 1);
		if (opcode == IR_OP_ADD)
		{
			operation = "fadd";
		}
		else if (opcode == IR_OP_SUBTRACT)
		{
			operation = "fsub";
		}
		else if (opcode == IR_OP_MULTIPLY)
		{
			operation = "fmul";
		}
		else if (opcode == IR_OP_DIVIDE_SIGNED)
		{
			operation = "fdiv";
		}
		if (operation == NULL)
		{
			error("PowerPC", "invalid floating-point IR operation %d", opcode);
		}
		fprintf(
		    power.output, "\t%s%s 0,0,1\n", operation, instruction->type.bits == 32U ? "s" : "");
		storeFloat(instruction->result, 0);
		return;
	}
	loadInteger(instruction->left, 3);
	loadInteger(instruction->right, 4);
	switch (opcode)
	{
	case IR_OP_ADD:
		operation = "add";
		break;
	case IR_OP_SUBTRACT:
		fputs("\tsubf 3,4,3\n", power.output);
		break;
	case IR_OP_MULTIPLY:
		operation = "mullw";
		break;
	case IR_OP_DIVIDE_SIGNED:
		operation = "divw";
		break;
	case IR_OP_DIVIDE_UNSIGNED:
		operation = "divwu";
		break;
	case IR_OP_BITWISE_AND:
		operation = "and";
		break;
	case IR_OP_BITWISE_OR:
		operation = "or";
		break;
	case IR_OP_BITWISE_XOR:
		operation = "xor";
		break;
	case IR_OP_SHIFT_LEFT:
		operation = "slw";
		break;
	case IR_OP_SHIFT_RIGHT_SIGNED:
		operation = "sraw";
		break;
	case IR_OP_SHIFT_RIGHT_UNSIGNED:
		operation = "srw";
		break;
	case IR_OP_REMAINDER_SIGNED:
	case IR_OP_REMAINDER_UNSIGNED:
		fprintf(power.output,
		        "\t%s 5,3,4\n\tmullw 5,5,4\n\tsubf 3,5,3\n",
		        opcode == IR_OP_REMAINDER_SIGNED ? "divw" : "divwu");
		break;
	default:
		error("PowerPC", "invalid binary IR operation %d", opcode);
	}
	if (operation != NULL)
	{
		fprintf(power.output, "\t%s 3,3,4\n", operation);
	}
	storeInteger(instruction->result, 3);
}

static void emitPointer(const IrInstruction *instruction)
{
	loadInteger(instruction->left, 3);
	loadInteger(instruction->right, 4);
	immediate(5, (uint32_t)instruction->offset);
	if (instruction->opcode == IR_OP_POINTER_DIFFERENCE)
	{
		fputs("\tsubf 3,4,3\n\tdivw 3,3,5\n", power.output);
	}
	else
	{
		fputs("\tmullw 4,4,5\n", power.output);
		fputs(instruction->opcode == IR_OP_POINTER_ADD ? "\tadd 3,3,4\n" : "\tsubf 3,4,3\n",
		      power.output);
	}
	storeInteger(instruction->result, 3);
}

static void emitParameter(const IrInstruction *instruction)
{
	PpcArgumentLocation *location = &power.frame.parameters[instruction->offset];
	IrType type = instruction->type;
	int offset;
	if (type.kind == IR_TYPE_FLOAT && location->floatingRegister >= 0)
	{
		offset = power.frame.incomingRegisters + 32 + (location->floatingRegister - 1) * 8;
		memory("lfd", 0, 1, offset);
		storeFloat(instruction->result, 0);
	}
	else if (location->generalRegister >= 0)
	{
		offset = power.frame.incomingRegisters + (location->generalRegister - 3) * 4;
		memory("lwz", 3, 1, offset);
		if (type.kind == IR_TYPE_AGGREGATE)
		{
			if (!power.abi->hasDescriptors)
			{
				memory(type.bits == 8U ? "lbz" : type.bits == 16U ? "lhz" : "lwz", 3, 3, 0);
			}
			else if (type.bits < 32U)
			{
				fprintf(power.output, "\tsrwi 3,3,%u\n", 32U - type.bits);
			}
		}
		storeInteger(instruction->result, 3);
	}
	else
	{
		offset = power.frame.frameSize + location->stackOffset;
		if (type.kind == IR_TYPE_FLOAT)
		{
			memory(type.bits == 32U ? "lfs" : "lfd", 0, 1, offset);
			storeFloat(instruction->result, 0);
		}
		else
		{
			memory("lwz", 3, 1, offset);
			if (type.kind == IR_TYPE_AGGREGATE && !power.abi->hasDescriptors)
			{
				memory(type.bits == 8U ? "lbz" : type.bits == 16U ? "lhz" : "lwz", 3, 3, 0);
			}
			else if (type.kind == IR_TYPE_AGGREGATE && type.bits < 32U)
			{
				fprintf(power.output, "\tsrwi 3,3,%u\n", 32U - type.bits);
			}
			storeInteger(instruction->result, 3);
		}
	}
}

static void emitCall(const IrInstruction *instruction)
{
	PpcArgumentCursor cursor = {0, 0, 0};
	int index;
	int copyOffset = power.frame.argumentCopies;
	if (instruction->type.kind == IR_TYPE_AGGREGATE)
	{
		address(3, 1, power.frame.values[instruction->result] + 4 - instruction->type.bits / 8);
		cursor.words = 1;
		if (power.abi->hasDescriptors)
		{
			memory("stw", 3, 1, 24);
		}
	}
	for (index = 0; index < instruction->argumentCount; ++index)
	{
		IrValueId argument = instruction->arguments[index];
		IrType type = power.frame.types[argument];
		PpcArgumentLocation location;
		power.abi->classify(&cursor, type, &location);
		if (type.kind == IR_TYPE_FLOAT)
		{
			loadFloat(argument, location.floatingRegister >= 0 ? location.floatingRegister : 0);
			if (location.stackOffset >= 0)
			{
				memory(type.bits == 32U ? "stfs" : "stfd",
				       location.floatingRegister >= 0 ? location.floatingRegister : 0,
				       1,
				       location.stackOffset);
			}
			if (power.abi->hasDescriptors && location.generalRegister >= 0)
			{
				memory("lwz", location.generalRegister, 1, location.stackOffset);
				if (location.words == 2 && location.generalRegister < 10)
				{
					memory("lwz", location.generalRegister + 1, 1, location.stackOffset + 4);
				}
			}
		}
		else
		{
			int reg = location.generalRegister >= 0 ? location.generalRegister : 12;
			if (type.kind == IR_TYPE_AGGREGATE && !power.abi->hasDescriptors)
			{
				loadInteger(argument, reg);
				memory("stw", reg, 1, copyOffset);
				address(reg, 1, copyOffset + 4 - type.bits / 8);
				copyOffset += 4;
			}
			else
			{
				loadInteger(argument, reg);
				if (type.kind == IR_TYPE_AGGREGATE && type.bits < 32U)
				{
					fprintf(power.output, "\tslwi %d,%d,%u\n", reg, reg, 32U - type.bits);
				}
			}
			if (location.stackOffset >= 0)
			{
				memory("stw", reg, 1, location.stackOffset);
			}
		}
	}
	if (!power.abi->hasDescriptors)
	{
		fputs(cursor.floating > 0 ? "\tcreqv 6,6,6\n" : "\tcrxor 6,6,6\n", power.output);
	}
	if (instruction->opcode == IR_OP_CALL_INDIRECT)
	{
		loadInteger(instruction->left, 12);
		power.abi->indirectCall(power.output);
	}
	else
	{
		char name[128];
		symbolName(instruction->symbol, name, sizeof(name));
		power.abi->directCall(power.output, name);
	}
	if (instruction->type.kind == IR_TYPE_AGGREGATE)
	{
		loadInteger(instruction->result, 3);
		storeInteger(instruction->result, 3);
	}
	else if (instruction->type.kind == IR_TYPE_FLOAT)
	{
		storeFloat(instruction->result, 1);
	}
	else if (instruction->type.kind != IR_TYPE_VOID && instruction->type.kind != IR_TYPE_AGGREGATE)
	{
		storeInteger(instruction->result, 3);
	}
}

static void emitVariadic(const IrInstruction *instruction)
{
	int words = power.abi->hasDescriptors ? 1 : 3;
	int index;
	loadInteger(instruction->left, 3);
	if (instruction->opcode == IR_OP_VA_START)
	{
		PpcArgumentCursor cursor = {0, 0, 0};
		cursor.words = power.frame.function->returnType.kind == IR_TYPE_AGGREGATE ? 1 : 0;
		for (index = 0; index < power.frame.function->parameterCount; ++index)
		{
			PpcArgumentLocation location;
			power.abi->classify(&cursor, power.frame.function->parameterTypes[index], &location);
		}
		if (power.abi->hasDescriptors)
		{
			address(4, 1, power.frame.frameSize + 24 + cursor.stack);
			memory("stw", 4, 3, 0);
		}
		else
		{
			immediate(4, ((uint32_t)cursor.words << 24) | ((uint32_t)cursor.floating << 16));
			memory("stw", 4, 3, 0);
			address(4, 1, power.frame.frameSize + 8 + cursor.stack);
			memory("stw", 4, 3, 4);
			address(4, 1, power.frame.incomingRegisters);
			memory("stw", 4, 3, 8);
		}
	}
	else if (instruction->opcode == IR_OP_VA_COPY || instruction->opcode == IR_OP_VA_END)
	{
		if (instruction->opcode == IR_OP_VA_COPY)
		{
			loadInteger(instruction->right, 4);
		}
		else
		{
			immediate(5, 0U);
		}
		for (index = 0; index < words; ++index)
		{
			if (instruction->opcode == IR_OP_VA_COPY)
			{
				memory("lwz", 5, 4, index * 4);
			}
			memory("stw", 5, 3, index * 4);
		}
	}
	else
	{
		int floating = instruction->type.kind == IR_TYPE_FLOAT;
		int size = instruction->type.bits > 32U ? 8 : 4;
		if (power.abi->hasDescriptors)
		{
			memory("lwz", 5, 3, 0);
			address(4, 5, size);
			memory("stw", 4, 3, 0);
		}
		else
		{
			unsigned int label = power.serial++;
			memory("lbz", 4, 3, floating ? 1 : 0);
			fprintf(power.output, "\tcmpwi 0,4,8\n\tbge L..CCvaoverflow%u\n", label);
			memory("lwz", 5, 3, 8);
			fprintf(power.output, "\tslwi 6,4,%d\n\tadd 5,5,6\n", floating ? 3 : 2);
			if (floating)
			{
				address(5, 5, 32);
			}
			address(4, 4, 1);
			memory("stb", 4, 3, floating ? 1 : 0);
			fprintf(power.output, "\tb L..CCvaload%u\nL..CCvaoverflow%u:\n", label, label);
			memory("lwz", 5, 3, 4);
			if (size == 8)
			{
				fputs("\taddi 5,5,7\n\tclrrwi 5,5,3\n", power.output);
			}
			address(4, 5, size);
			memory("stw", 4, 3, 4);
			fprintf(power.output, "L..CCvaload%u:\n", label);
		}
		if (floating)
		{
			memory(size == 8 ? "lfd" : "lfs", 0, 5, 0);
			storeFloat(instruction->result, 0);
		}
		else
		{
			if (instruction->type.kind == IR_TYPE_AGGREGATE)
			{
				if (!power.abi->hasDescriptors)
				{
					memory("lwz", 5, 5, 0);
				}
				memory(instruction->type.bits == 8U    ? "lbz"
				       : instruction->type.bits == 16U ? "lhz"
				                                       : "lwz",
				       3,
				       5,
				       0);
			}
			else
			{
				memory("lwz", 3, 5, 0);
			}
			storeInteger(instruction->result, 3);
		}
	}
}

static void emitInstruction(const IrInstruction *instruction)
{
	IrOpcode opcode = instruction->opcode;
	IrType type = instruction->type;
	if (opcode >= IR_OP_ADD && opcode <= IR_OP_SHIFT_RIGHT_UNSIGNED)
	{
		emitBinary(instruction);
		return;
	}
	switch (opcode)
	{
	case IR_OP_PARAMETER:
		emitParameter(instruction);
		break;
	case IR_OP_UNDEFINED:
		/* An indeterminate C value has no initialization obligation. */
		break;
	case IR_OP_CONSTANT_INTEGER:
		immediate(3, instruction->integer);
		storeInteger(instruction->result, 3);
		break;
	case IR_OP_CONSTANT_FLOAT:
		floatingConstant(instruction->floating, 0);
		storeFloat(instruction->result, 0);
		break;
	case IR_OP_ADDRESS_OF:
	{
		char name[128];
		symbolName(instruction->symbol, name, sizeof(name));
		power.abi->symbolAddress(power.output, 3, name, power.serial++);
		address(3, 3, instruction->offset);
		storeInteger(instruction->result, 3);
		break;
	}
	case IR_OP_LOCAL_ADDRESS:
		address(3, 1, power.frame.locals[instruction->local] + instruction->offset);
		storeInteger(instruction->result, 3);
		break;
	case IR_OP_LOAD:
		loadInteger(instruction->left, 3);
		if (type.kind == IR_TYPE_FLOAT)
		{
			memory(type.bits == 32U ? "lfs" : "lfd", 0, 3, 0);
			storeFloat(instruction->result, 0);
		}
		else
		{
			memory(type.bits == 8U ? "lbz" : type.bits == 16U ? "lhz" : "lwz", 3, 3, 0);
			storeInteger(instruction->result, 3);
		}
		break;
	case IR_OP_STORE:
		loadInteger(instruction->left, 3);
		if (type.kind == IR_TYPE_FLOAT)
		{
			loadFloat(instruction->right, 0);
			memory(type.bits == 32U ? "stfs" : "stfd", 0, 3, 0);
		}
		else
		{
			loadInteger(instruction->right, 4);
			memory(type.bits == 8U ? "stb" : type.bits == 16U ? "sth" : "stw", 4, 3, 0);
		}
		break;
	case IR_OP_ZERO_MEMORY:
		if (instruction->offset > 0)
		{
			unsigned int label = power.serial++;
			loadInteger(instruction->left, 3);
			immediate(4, (uint32_t)instruction->offset);
			fprintf(power.output,
			        "\tmtctr 4\n\tli 4,0\nL..CCzero%u:\n"
			        "\tstb 4,0(3)\n\taddi 3,3,1\n\tbdnz L..CCzero%u\n",
			        label,
			        label);
		}
		break;
	case IR_OP_COPY:
	case IR_OP_NEGATE:
	case IR_OP_BITWISE_NOT:
		if (type.kind == IR_TYPE_FLOAT)
		{
			loadFloat(instruction->left, 0);
			if (opcode == IR_OP_NEGATE)
			{
				fputs("\tfneg 0,0\n", power.output);
			}
			storeFloat(instruction->result, 0);
		}
		else
		{
			loadInteger(instruction->left, 3);
			if (opcode == IR_OP_NEGATE)
			{
				fputs("\tneg 3,3\n", power.output);
			}
			else if (opcode == IR_OP_BITWISE_NOT)
			{
				fputs("\tnor 3,3,3\n", power.output);
			}
			storeInteger(instruction->result, 3);
		}
		break;
	case IR_OP_POINTER_ADD:
	case IR_OP_POINTER_SUBTRACT:
	case IR_OP_POINTER_DIFFERENCE:
		emitPointer(instruction);
		break;
	case IR_OP_CONVERT:
		emitConversion(instruction);
		break;
	case IR_OP_COMPARE:
		emitComparison(instruction);
		break;
	case IR_OP_CALL:
	case IR_OP_CALL_INDIRECT:
		emitCall(instruction);
		break;
	case IR_OP_VA_START:
	case IR_OP_VA_ARGUMENT:
	case IR_OP_VA_COPY:
	case IR_OP_VA_END:
		emitVariadic(instruction);
		break;
	case IR_OP_BRANCH:
		fprintf(power.output, "\tb L..CCblock%d_%d\n", power.frame.ordinal, instruction->trueBlock);
		break;
	case IR_OP_BRANCH_CONDITIONAL:
	{
		unsigned int label = power.serial++;
		loadInteger(instruction->left, 3);
		fprintf(power.output,
		        "\tcmpwi 0,3,0\n\tbeq L..CCfalse%u\n"
		        "\tb L..CCblock%d_%d\nL..CCfalse%u:\n\tb L..CCblock%d_%d\n",
		        label,
		        power.frame.ordinal,
		        instruction->trueBlock,
		        label,
		        power.frame.ordinal,
		        instruction->falseBlock);
		break;
	}
	case IR_OP_RETURN:
		if (type.kind == IR_TYPE_FLOAT)
		{
			loadFloat(instruction->left, 1);
		}
		else if (type.kind != IR_TYPE_VOID)
		{
			loadInteger(instruction->left, 3);
			if (type.kind == IR_TYPE_AGGREGATE)
			{
				memory("lwz", 4, 1, power.frame.incomingRegisters);
				memory(type.bits == 8U ? "stb" : type.bits == 16U ? "sth" : "stw", 3, 4, 0);
			}
		}
		fputs("\tlwz 1,0(1)\n", power.output);
		fprintf(power.output, "\tlwz 0,%d(1)\n\tmtlr 0\n\tblr\n", power.abi->savedLinkOffset);
		break;
	default:
		error("PowerPC", "unsupported IR opcode %d", opcode);
	}
}

static void layoutFunction(const IrFunction *function)
{
	int size = power.abi->minimumArgumentSize;
	int copyBytes = 0;
	int blockIndex;
	int index;
	PpcArgumentCursor incoming = {0, 0, 0};
	incoming.words = function->returnType.kind == IR_TYPE_AGGREGATE ? 1 : 0;
	power.frame.function = function;
	power.frame.types = irCollectValueTypes(function);
	power.frame.values = xalloc((size_t)function->nextValue * sizeof(int));
	power.frame.locals = xalloc((size_t)function->localCount * sizeof(int));
	power.frame.parameters = xalloc((size_t)function->parameterCount * sizeof(PpcArgumentLocation));
	for (index = 0; index < function->parameterCount; ++index)
	{
		power.abi->classify(
		    &incoming, function->parameterTypes[index], &power.frame.parameters[index]);
	}
	for (blockIndex = 0; blockIndex < function->blockCount; ++blockIndex)
	{
		const IrBasicBlock *block = &function->blocks[blockIndex];
		for (index = 0; index < block->instructionCount; ++index)
		{
			const IrInstruction *instruction = &block->instructions[index];
			if (instruction->opcode == IR_OP_CALL || instruction->opcode == IR_OP_CALL_INDIRECT)
			{
				PpcArgumentCursor cursor = {0, 0, 0};
				int argument;
				int callCopies = 0;
				cursor.words = instruction->type.kind == IR_TYPE_AGGREGATE ? 1 : 0;
				for (argument = 0; argument < instruction->argumentCount; ++argument)
				{
					PpcArgumentLocation location;
					power.abi->classify(
					    &cursor, power.frame.types[instruction->arguments[argument]], &location);
					if (power.frame.types[instruction->arguments[argument]].kind ==
					    IR_TYPE_AGGREGATE)
					{
						callCopies += 4;
					}
				}
				if (cursor.stack > size)
				{
					size = cursor.stack;
				}
				if (callCopies > copyBytes)
				{
					copyBytes = callCopies;
				}
			}
		}
	}
	size = alignOffset(size + power.abi->linkageSize, 8);
	power.frame.argumentCopies = size;
	size = alignOffset(size + copyBytes, 8);
	power.frame.incomingRegisters = size;
	size += 32 + power.abi->floatingRegisterCount * 8;
	power.frame.scratch = alignOffset(size, 8);
	size = power.frame.scratch + 24;
	for (index = 0; index < function->localCount; ++index)
	{
		const IrLocal *local = &function->locals[index];
		if (local->size > (size_t)(INT_MAX - size - 16))
		{
			error("PowerPC", "local object exceeds stack address range");
		}
		size = alignOffset(size, local->alignment);
		power.frame.locals[index] = size;
		size += (int)local->size;
	}
	for (index = 0; index < function->nextValue; ++index)
	{
		int width = power.frame.types[index].bits > 32U ? 8 : 4;
		size = alignOffset(size, width);
		power.frame.values[index] = size;
		size += width;
	}
	power.frame.frameSize = alignOffset(size, 16);
}

static void emitFunction(const IrFunction *function, int ordinal)
{
	char name[128];
	int index;
	int blockIndex;
	memset(&power.frame, 0, sizeof(power.frame));
	power.frame.ordinal = ordinal;
	layoutFunction(function);
	symbolName(function->symbol, name, sizeof(name));
	power.abi->functionEntry(power.output, name, function->isInternal);
	fprintf(power.output, "\tmflr 0\n\tstw 0,%d(1)\n", power.abi->savedLinkOffset);
	if (power.frame.frameSize <= 32768)
	{
		fprintf(power.output, "\tstwu 1,-%d(1)\n", power.frame.frameSize);
	}
	else
	{
		immediate(0, (uint32_t)-power.frame.frameSize);
		fputs("\tstwux 1,1,0\n", power.output);
	}
	for (index = 0; index < 8; ++index)
	{
		memory("stw", index + 3, 1, power.frame.incomingRegisters + index * 4);
		if (power.abi->hasDescriptors && function->isVariadic)
		{
			memory("stw", index + 3, 1, power.frame.frameSize + 24 + index * 4);
		}
	}
	for (index = 0; index < power.abi->floatingRegisterCount; ++index)
	{
		memory("stfd", index + 1, 1, power.frame.incomingRegisters + 32 + index * 8);
	}
	for (blockIndex = 0; blockIndex < function->blockCount; ++blockIndex)
	{
		const IrBasicBlock *block = &function->blocks[blockIndex];
		fprintf(power.output, "L..CCblock%d_%d:\n", ordinal, block->id);
		for (index = 0; index < block->instructionCount; ++index)
		{
			emitInstruction(&block->instructions[index]);
		}
	}
	power.abi->functionEnd(power.output, name);
	free(power.frame.parameters);
	free(power.frame.locals);
	free(power.frame.values);
	free(power.frame.types);
}

static void emitGlobal(const IrGlobal *global)
{
	char name[128];
	size_t offset;
	size_t size = global->zeroFillSize;
	int alignment = 0;
	int width = global->alignment;
	if (global->isExternal)
	{
		return;
	}
	while (width > 1)
	{
		++alignment;
		width >>= 1;
	}
	symbolName(global->symbol, name, sizeof(name));
	if (power.abi->hasDescriptors)
	{
		fprintf(power.output, "\t.csect %s[RW],%d\n", name, alignment);
		if (!global->isInternal)
		{
			fprintf(power.output, "\t.globl %s\n", name);
		}
	}
	else
	{
		fprintf(power.output, "\t.data\n\t.p2align %d\n", alignment);
		if (!global->isInternal)
		{
			fprintf(power.output, "\t.globl %s\n", name);
		}
		fprintf(power.output,
		        "\t.type %s,@object\n\t.size %s,%u\n%s:\n",
		        name,
		        name,
		        (unsigned int)size,
		        name);
	}
	for (offset = 0; offset < global->initializerSize;)
	{
		int relocationIndex;
		const IrRelocation *relocation = NULL;
		for (relocationIndex = 0; relocationIndex < global->relocationCount; ++relocationIndex)
		{
			if (global->relocations[relocationIndex].offset == offset)
			{
				relocation = &global->relocations[relocationIndex];
				break;
			}
		}
		if (relocation != NULL)
		{
			char destination[128];
			symbolName(relocation->symbol, destination, sizeof(destination));
			fprintf(power.output, "\t.long %s%+d\n", destination, relocation->addend);
			offset += 4U;
		}
		else
		{
			fprintf(power.output, "\t.byte %u\n", (unsigned int)global->initializer[offset++]);
		}
	}
	if (size > global->initializerSize)
	{
		fprintf(power.output, "\t.space %u,0\n", (unsigned int)(size - global->initializerSize));
	}
}

static void declareExternal(IrSymbolId symbol, unsigned char *declared)
{
	int index;
	char name[128];
	Name *function;
	if (symbol < 0 || symbol >= cd.hash.size || declared[symbol])
	{
		return;
	}
	declared[symbol] = 1U;
	for (index = 0; index < compiler.ir.functionCount; ++index)
	{
		if (compiler.ir.functions[index].symbol == symbol)
		{
			return;
		}
	}
	for (index = 0; index < compiler.ir.globalCount; ++index)
	{
		if (compiler.ir.globals[index].symbol == symbol && !compiler.ir.globals[index].isExternal)
		{
			return;
		}
	}
	symbolName(symbol, name, sizeof(name));
	function = getNameFromAllTable(globTable, NM_FUNC, symbol);
	if (function != NULL)
	{
		fprintf(power.output, "\t.extern .%s[PR]\n\t.extern %s[DS]\n", name, name);
	}
	else
	{
		fprintf(power.output, "\t.extern %s[RW]\n", name);
	}
}

static void declareAixExternals(void)
{
	unsigned char *declared = xalloc((size_t)cd.hash.size);
	int functionIndex;
	int globalIndex;
	for (functionIndex = 0; functionIndex < compiler.ir.functionCount; ++functionIndex)
	{
		const IrFunction *function = &compiler.ir.functions[functionIndex];
		int blockIndex;
		for (blockIndex = 0; blockIndex < function->blockCount; ++blockIndex)
		{
			const IrBasicBlock *block = &function->blocks[blockIndex];
			int index;
			for (index = 0; index < block->instructionCount; ++index)
			{
				const IrInstruction *instruction = &block->instructions[index];
				if (instruction->opcode == IR_OP_CALL || instruction->opcode == IR_OP_ADDRESS_OF)
				{
					declareExternal(instruction->symbol, declared);
				}
			}
		}
	}
	for (globalIndex = 0; globalIndex < compiler.ir.globalCount; ++globalIndex)
	{
		const IrGlobal *global = &compiler.ir.globals[globalIndex];
		int index;
		for (index = 0; index < global->relocationCount; ++index)
		{
			declareExternal(global->relocations[index].symbol, declared);
		}
	}
	free(declared);
}

void ppcEmitModule(const char *path, const PpcAbi *abi)
{
	int index;
	irVerifyModule(&compiler.ir);
	memset(&power, 0, sizeof(power));
	power.abi = abi;
	power.output = fopen(path, "w");
	if (power.output == NULL)
	{
		error("PowerPC", "cannot create '%s'", path);
	}
	fprintf(power.output, "# cc: big-endian PowerPC32, %s\n", abi->name);
	if (abi->hasDescriptors)
	{
		declareAixExternals();
	}
	for (index = 0; index < compiler.ir.functionCount; ++index)
	{
		emitFunction(&compiler.ir.functions[index], index);
	}
	for (index = 0; index < compiler.ir.globalCount; ++index)
	{
		emitGlobal(&compiler.ir.globals[index]);
	}
	if (!abi->hasDescriptors)
	{
		fputs("\t.section .note.GNU-stack,\"\",@progbits\n", power.output);
	}
	if (ferror(power.output) || fclose(power.output) != 0)
	{
		error("PowerPC", "failed writing '%s'", path);
	}
}

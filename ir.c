/* Storage, verification, and deterministic printing for the neutral IR. */

#include "cc.h"

static int isTerminator(IrOpcode opcode)
{
	return opcode == IR_OP_BRANCH || opcode == IR_OP_BRANCH_CONDITIONAL || opcode == IR_OP_RETURN;
}

static int producesValue(IrOpcode opcode)
{
	return opcode != IR_OP_STORE && opcode != IR_OP_BRANCH && opcode != IR_OP_BRANCH_CONDITIONAL &&
	       opcode != IR_OP_RETURN;
}

static int usesLeft(IrOpcode opcode)
{
	switch (opcode)
	{
	case IR_OP_LOAD:
	case IR_OP_STORE:
	case IR_OP_ZERO_MEMORY:
	case IR_OP_COPY:
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
	case IR_OP_NEGATE:
	case IR_OP_BITWISE_NOT:
	case IR_OP_COMPARE:
	case IR_OP_CONVERT:
	case IR_OP_CALL_INDIRECT:
	case IR_OP_VA_START:
	case IR_OP_VA_ARGUMENT:
	case IR_OP_VA_COPY:
	case IR_OP_VA_END:
	case IR_OP_BRANCH_CONDITIONAL:
	case IR_OP_RETURN:
		return 1;
	default:
		return 0;
	}
}

static int usesRight(IrOpcode opcode)
{
	switch (opcode)
	{
	case IR_OP_STORE:
	case IR_OP_VA_COPY:
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
	case IR_OP_COMPARE:
		return 1;
	default:
		return 0;
	}
}

static int blockExists(const IrFunction *function, IrBlockId id)
{
	int index;
	for (index = 0; index < function->blockCount; ++index)
	{
		if (function->blocks[index].id == id)
		{
			return 1;
		}
	}
	return 0;
}

static const char *opcodeName(IrOpcode opcode)
{
	static const char *names[] = {
	    "parameter",
	    "undefined",
	    "constant_integer",
	    "constant_float",
	    "address_of",
	    "local_address",
	    "load",
	    "store",
	    "zero_memory",
	    "copy",
	    "add",
	    "subtract",
	    "multiply",
	    "sdiv",
	    "udiv",
	    "srem",
	    "urem",
	    "and",
	    "or",
	    "xor",
	    "shl",
	    "ashr",
	    "lshr",
	    "pointer_add",
	    "pointer_subtract",
	    "pointer_difference",
	    "negate",
	    "not",
	    "compare",
	    "convert",
	    "call",
	    "call_indirect",
	    "va_start",
	    "va_arg",
	    "va_copy",
	    "va_end",
	    "branch",
	    "branch_if",
	    "return",
	};
	if (opcode < IR_OP_PARAMETER || opcode > IR_OP_RETURN)
	{
		error("IR", "invalid opcode %d", opcode);
	}
	return names[opcode];
}

void irModuleInit(IrModule *module)
{
	if (module == NULL)
	{
		error("IR", "cannot initialize a null module");
	}
	memset(module, 0, sizeof(*module));
	module->nextAnonymousSymbol = -2;
}

IrSymbolId irCreateAnonymousSymbol(IrModule *module)
{
	IrSymbolId symbol;
	if (module == NULL || module->nextAnonymousSymbol == INT_MIN)
	{
		error("IR", "anonymous symbol space exhausted");
	}
	symbol = module->nextAnonymousSymbol;
	--module->nextAnonymousSymbol;
	return symbol;
}

IrGlobal *irAddGlobal(IrModule *module,
                      IrSymbolId symbol,
                      IrType type,
                      size_t size,
                      int alignment,
                      int isExternal,
                      int isExported)
{
	IrGlobal *global;
	int index;
	if (module == NULL || symbol == IR_SYMBOL_NONE || alignment <= 0 || (size == 0U && !isExternal))
	{
		error("IR", "invalid global declaration");
	}
	for (index = 0; index < module->globalCount; ++index)
	{
		if (module->globals[index].symbol == symbol)
		{
			error("IR", "duplicate global symbol %d", symbol);
		}
	}
	if (module->globalCount == module->globalCapacity)
	{
		int capacity = module->globalCapacity == 0 ? 16 : module->globalCapacity * 2;
		module->globals = xrealloc(module->globals, (size_t)capacity * sizeof(*module->globals));
		memset(module->globals + module->globalCapacity,
		       0,
		       (size_t)(capacity - module->globalCapacity) * sizeof(*module->globals));
		module->globalCapacity = capacity;
	}
	global = &module->globals[module->globalCount++];
	memset(global, 0, sizeof(*global));
	global->symbol = symbol;
	global->type = type;
	global->zeroFillSize = size;
	global->alignment = alignment;
	global->isExternal = isExternal != 0;
	global->isExported = isExported != 0;
	return global;
}

void irSetGlobalInitializer(IrGlobal *global, const void *bytes, size_t size)
{
	if (global == NULL || global->isExternal || size > global->zeroFillSize ||
	    (size > 0U && bytes == NULL))
	{
		error("IR", "invalid global initializer");
	}
	free(global->initializer);
	global->initializer = NULL;
	global->initializerSize = size;
	if (size > 0U)
	{
		global->initializer = xalloc(size);
		memcpy(global->initializer, bytes, size);
	}
}

IrGlobal *irFindGlobal(IrModule *module, IrSymbolId symbol)
{
	int index;
	if (module == NULL || symbol == IR_SYMBOL_NONE)
	{
		return NULL;
	}
	for (index = 0; index < module->globalCount; ++index)
	{
		if (module->globals[index].symbol == symbol)
		{
			return &module->globals[index];
		}
	}
	return NULL;
}

void irAddGlobalRelocation(IrGlobal *global, size_t offset, IrSymbolId symbol, int addend)
{
	IrRelocation *relocation;
	if (global == NULL || global->isExternal || symbol == IR_SYMBOL_NONE ||
	    offset > global->zeroFillSize || global->zeroFillSize - offset < 4U)
	{
		error("IR", "invalid global relocation");
	}
	if (global->relocationCount > 0 &&
	    global->relocations[global->relocationCount - 1].offset + 4U > offset)
	{
		error("IR", "overlapping or unordered global relocation");
	}
	if (global->relocationCount == global->relocationCapacity)
	{
		int capacity = global->relocationCapacity == 0 ? 4 : global->relocationCapacity * 2;

		global->relocations =
		    xrealloc(global->relocations, (size_t)capacity * sizeof(*global->relocations));
		global->relocationCapacity = capacity;
	}
	relocation = &global->relocations[global->relocationCount++];
	relocation->offset = offset;
	relocation->symbol = symbol;
	relocation->addend = addend;
}

void irModuleFree(IrModule *module)
{
	int functionIndex;
	if (module == NULL)
	{
		return;
	}
	for (functionIndex = 0; functionIndex < module->functionCount; ++functionIndex)
	{
		IrFunction *function = &module->functions[functionIndex];
		int blockIndex;
		for (blockIndex = 0; blockIndex < function->blockCount; ++blockIndex)
		{
			IrBasicBlock *block = &function->blocks[blockIndex];
			int instructionIndex;
			for (instructionIndex = 0; instructionIndex < block->instructionCount;
			     ++instructionIndex)
			{
				free(block->instructions[instructionIndex].arguments);
			}
			free(block->instructions);
		}
		free(function->blocks);
		free(function->parameterTypes);
		free(function->locals);
	}
	for (functionIndex = 0; functionIndex < module->globalCount; ++functionIndex)
	{
		free(module->globals[functionIndex].initializer);
		free(module->globals[functionIndex].relocations);
	}
	free(module->functions);
	free(module->globals);
	memset(module, 0, sizeof(*module));
}

static IrType makeType(IrTypeKind kind, unsigned int bits, int isUnsigned, unsigned int alignment)
{
	IrType type;
	if (bits > UCHAR_MAX || alignment > UCHAR_MAX)
	{
		error("IR type", "type width or alignment is outside the supported range");
	}
	type.kind = (unsigned char)kind;
	type.bits = (unsigned char)bits;
	type.isUnsigned = (unsigned char)(isUnsigned != 0);
	type.alignment = (unsigned char)alignment;
	return type;
}

IrType irTypeVoid(void)
{
	return makeType(IR_TYPE_VOID, 0U, 0, 0U);
}

IrType irTypeInteger(unsigned int bits, int isUnsigned, unsigned int alignment)
{
	if (bits == 0U || alignment == 0U)
	{
		error("IR type", "integer type requires a width and alignment");
	}
	return makeType(IR_TYPE_INTEGER, bits, isUnsigned, alignment);
}

IrType irTypeFloat(unsigned int bits, unsigned int alignment)
{
	if (bits == 0U || alignment == 0U)
	{
		error("IR type", "floating type requires a width and alignment");
	}
	return makeType(IR_TYPE_FLOAT, bits, 0, alignment);
}

IrType irTypePointer(unsigned int bits, unsigned int alignment)
{
	if (bits == 0U || alignment == 0U)
	{
		error("IR type", "pointer type requires a width and alignment");
	}
	return makeType(IR_TYPE_POINTER, bits, 1, alignment);
}

int irTypesEqual(IrType left, IrType right)
{
	return left.kind == right.kind && left.bits == right.bits &&
	       left.isUnsigned == right.isUnsigned && left.alignment == right.alignment;
}

IrFunction *irAddFunction(IrModule *module, IrSymbolId symbol, IrType returnType)
{
	IrFunction *function;
	int index;
	if (module == NULL || symbol == IR_SYMBOL_NONE)
	{
		error("IR", "invalid function declaration");
	}
	for (index = 0; index < module->functionCount; ++index)
	{
		if (module->functions[index].symbol == symbol)
		{
			error("IR", "duplicate function symbol %d", symbol);
		}
	}
	if (module->functionCount == module->functionCapacity)
	{
		int capacity = module->functionCapacity == 0 ? 8 : module->functionCapacity * 2;
		module->functions =
		    xrealloc(module->functions, (size_t)capacity * sizeof(*module->functions));
		memset(module->functions + module->functionCapacity,
		       0,
		       (size_t)(capacity - module->functionCapacity) * sizeof(*module->functions));
		module->functionCapacity = capacity;
	}
	function = &module->functions[module->functionCount++];
	memset(function, 0, sizeof(*function));
	function->symbol = symbol;
	function->returnType = returnType;
	return function;
}

void irSetFunctionParameters(IrFunction *function, const IrType *types, int count)
{
	if (function == NULL || count < 0 || (count > 0 && types == NULL) ||
	    function->parameterTypes != NULL)
	{
		error("IR", "invalid function parameter list");
	}
	if (function->blockCount != 0 || function->nextValue != 0)
	{
		error("IR", "function signature must be completed before its body");
	}
	function->parameterCount = count;
	if (count > 0)
	{
		function->parameterTypes = xalloc((size_t)count * sizeof(*function->parameterTypes));
		memcpy(function->parameterTypes, types, (size_t)count * sizeof(*function->parameterTypes));
	}
}

IrBasicBlock *irAddBlock(IrFunction *function)
{
	IrBasicBlock *block;
	if (function->blockCount == function->blockCapacity)
	{
		int capacity = function->blockCapacity == 0 ? 8 : function->blockCapacity * 2;
		function->blocks = xrealloc(function->blocks, (size_t)capacity * sizeof(*function->blocks));
		memset(function->blocks + function->blockCapacity,
		       0,
		       (size_t)(capacity - function->blockCapacity) * sizeof(*function->blocks));
		function->blockCapacity = capacity;
	}
	block = &function->blocks[function->blockCount];
	memset(block, 0, sizeof(*block));
	block->id = function->blockCount++;
	return block;
}

IrLocalId irAddLocal(IrFunction *function,
                     IrSymbolId symbol,
                     IrType type,
                     size_t size,
                     int alignment,
                     int parameterIndex)
{
	IrLocal *local;
	if (function == NULL || size == 0U || alignment <= 0 ||
	    (parameterIndex < -1 || parameterIndex >= function->parameterCount))
	{
		error("IR", "invalid local storage declaration");
	}
	if (function->localCount == function->localCapacity)
	{
		int capacity = function->localCapacity == 0 ? 8 : function->localCapacity * 2;
		function->locals = xrealloc(function->locals, (size_t)capacity * sizeof(*function->locals));
		memset(function->locals + function->localCapacity,
		       0,
		       (size_t)(capacity - function->localCapacity) * sizeof(*function->locals));
		function->localCapacity = capacity;
	}
	local = &function->locals[function->localCount];
	local->id = function->localCount++;
	local->symbol = symbol;
	local->type = type;
	local->size = size;
	local->alignment = alignment;
	local->parameterIndex = parameterIndex;
	return local->id;
}

IrValueId irNextValue(IrFunction *function)
{
	return function->nextValue++;
}

IrInstruction *irAppendInstruction(IrBasicBlock *block, IrOpcode opcode, IrType type)
{
	IrInstruction *instruction;
	if (block->instructionCount > 0 &&
	    isTerminator(block->instructions[block->instructionCount - 1].opcode))
	{
		error("IR", "cannot append an instruction after a block terminator");
	}
	if (block->instructionCount == block->instructionCapacity)
	{
		int capacity = block->instructionCapacity == 0 ? 16 : block->instructionCapacity * 2;
		block->instructions =
		    xrealloc(block->instructions, (size_t)capacity * sizeof(*block->instructions));
		memset(block->instructions + block->instructionCapacity,
		       0,
		       (size_t)(capacity - block->instructionCapacity) * sizeof(*block->instructions));
		block->instructionCapacity = capacity;
	}
	instruction = &block->instructions[block->instructionCount++];
	memset(instruction, 0, sizeof(*instruction));
	instruction->opcode = opcode;
	instruction->type = type;
	instruction->result = IR_VALUE_NONE;
	instruction->left = IR_VALUE_NONE;
	instruction->right = IR_VALUE_NONE;
	instruction->symbol = IR_SYMBOL_NONE;
	instruction->local = IR_LOCAL_NONE;
	instruction->trueBlock = IR_BLOCK_NONE;
	instruction->falseBlock = IR_BLOCK_NONE;
	instruction->sourceFile = -1;
	instruction->sourceLine = -1;
	return instruction;
}

static void requireBuilder(const IrBuilder *builder)
{
	if (builder == NULL || builder->module == NULL || builder->function == NULL ||
	    builder->block < 0 || builder->block >= builder->function->blockCount)
	{
		error("IR builder", "no active function and basic block");
	}
}

static IrInstruction *appendBuiltInstruction(IrBuilder *builder, IrOpcode opcode, IrType type)
{
	IrInstruction *instruction;
	requireBuilder(builder);
	instruction = irAppendInstruction(&builder->function->blocks[builder->block], opcode, type);
	instruction->sourceFile = builder->sourceFile;
	instruction->sourceLine = builder->sourceLine;
	return instruction;
}

static IrValueId finishValue(IrBuilder *builder, IrInstruction *instruction)
{
	instruction->result = irNextValue(builder->function);
	return instruction->result;
}

void irBuilderInit(IrBuilder *builder, IrModule *module)
{
	if (builder == NULL || module == NULL)
	{
		error("IR builder", "cannot initialize a null builder or module");
	}
	memset(builder, 0, sizeof(*builder));
	builder->module = module;
	builder->block = IR_BLOCK_NONE;
	builder->sourceFile = -1;
	builder->sourceLine = -1;
}

void irBuilderSetSource(IrBuilder *builder, int sourceFile, int sourceLine)
{
	if (builder == NULL)
	{
		error("IR builder", "cannot set source location on a null builder");
	}
	builder->sourceFile = sourceFile;
	builder->sourceLine = sourceLine;
}

void irBuilderSuspend(IrBuilder *builder)
{
	if (builder == NULL || builder->suspensionDepth == INT_MAX)
	{
		error("IR builder", "invalid suspension request");
	}
	++builder->suspensionDepth;
}

void irBuilderResume(IrBuilder *builder)
{
	if (builder == NULL || builder->suspensionDepth <= 0)
	{
		error("IR builder", "unbalanced resume request");
	}
	--builder->suspensionDepth;
}

IrFunction *irBuilderBeginFunction(IrBuilder *builder,
                                   IrSymbolId symbol,
                                   IrType returnType,
                                   const IrType *parameterTypes,
                                   int parameterCount)
{
	IrBasicBlock *entryBlock;

	if (builder == NULL || builder->module == NULL || builder->function != NULL)
	{
		error("IR builder", "invalid function begin");
	}
	builder->function = irAddFunction(builder->module, symbol, returnType);
	irSetFunctionParameters(builder->function, parameterTypes, parameterCount);
	entryBlock = irAddBlock(builder->function);
	builder->block = entryBlock->id;
	return builder->function;
}

void irBuilderEndFunction(IrBuilder *builder)
{
	if (builder == NULL || builder->function == NULL)
	{
		error("IR builder", "no active function");
	}
	requireBuilder(builder);
	if (!irBuilderBlockTerminated(builder))
	{
		error("IR builder", "function %d ends in an unterminated block", builder->function->symbol);
	}
	builder->function = NULL;
	builder->block = IR_BLOCK_NONE;
}

IrBlockId irBuilderCreateBlock(IrBuilder *builder)
{
	IrBasicBlock *block;

	if (builder == NULL || builder->function == NULL)
	{
		error("IR builder", "cannot create a block outside a function");
	}
	block = irAddBlock(builder->function);
	return block->id;
}

void irBuilderSetBlock(IrBuilder *builder, IrBlockId block)
{
	if (builder == NULL || builder->function == NULL || block < 0 ||
	    block >= builder->function->blockCount)
	{
		error("IR builder", "invalid basic block selection");
	}
	builder->block = block;
}

int irBuilderBlockTerminated(const IrBuilder *builder)
{
	requireBuilder(builder);
	return builder->function->blocks[builder->block].instructionCount > 0 &&
	       isTerminator(
	           builder->function->blocks[builder->block]
	               .instructions[builder->function->blocks[builder->block].instructionCount - 1]
	               .opcode);
}

IrValueId irBuilderEmitParameter(IrBuilder *builder, IrType type, int parameterIndex)
{
	IrInstruction *instruction = appendBuiltInstruction(builder, IR_OP_PARAMETER, type);
	if (parameterIndex < 0 || parameterIndex >= builder->function->parameterCount)
	{
		error("IR builder", "parameter index %d is outside the function signature", parameterIndex);
	}
	instruction->offset = parameterIndex;
	return finishValue(builder, instruction);
}

IrValueId irBuilderEmitUndefined(IrBuilder *builder, IrType type)
{
	IrInstruction *instruction;
	if (type.kind == IR_TYPE_VOID)
	{
		error("IR builder", "undefined value cannot have void type");
	}
	instruction = appendBuiltInstruction(builder, IR_OP_UNDEFINED, type);
	return finishValue(builder, instruction);
}

IrValueId irBuilderEmitInteger(IrBuilder *builder, IrType type, uint32_t value)
{
	IrInstruction *instruction;
	if (type.kind != IR_TYPE_INTEGER && type.kind != IR_TYPE_POINTER &&
	    !(type.kind == IR_TYPE_AGGREGATE && value == 0U))
	{
		error("IR builder", "integer constant has a non-integral type");
	}
	instruction = appendBuiltInstruction(builder, IR_OP_CONSTANT_INTEGER, type);
	instruction->integer = value;
	return finishValue(builder, instruction);
}

IrValueId irBuilderEmitFloat(IrBuilder *builder, IrType type, double value)
{
	IrInstruction *instruction;
	if (type.kind != IR_TYPE_FLOAT)
	{
		error("IR builder", "floating constant has a non-floating type");
	}
	instruction = appendBuiltInstruction(builder, IR_OP_CONSTANT_FLOAT, type);
	instruction->floating = value;
	return finishValue(builder, instruction);
}

IrValueId irBuilderEmitSymbolAddress(IrBuilder *builder, IrType type, IrSymbolId symbol, int offset)
{
	IrInstruction *instruction;
	if (type.kind != IR_TYPE_POINTER || symbol == IR_SYMBOL_NONE)
	{
		error("IR builder", "invalid symbol address");
	}
	instruction = appendBuiltInstruction(builder, IR_OP_ADDRESS_OF, type);
	instruction->symbol = symbol;
	instruction->offset = offset;
	return finishValue(builder, instruction);
}

IrValueId irBuilderEmitLocalAddress(IrBuilder *builder, IrType type, IrLocalId local, int offset)
{
	IrInstruction *instruction;
	if (type.kind != IR_TYPE_POINTER || local < 0 || local >= builder->function->localCount)
	{
		error("IR builder", "invalid local address");
	}
	instruction = appendBuiltInstruction(builder, IR_OP_LOCAL_ADDRESS, type);
	instruction->local = local;
	instruction->offset = offset;
	return finishValue(builder, instruction);
}

IrValueId irBuilderEmitUnary(IrBuilder *builder, IrOpcode opcode, IrType type, IrValueId operand)
{
	IrInstruction *instruction;
	if (opcode != IR_OP_COPY && opcode != IR_OP_NEGATE && opcode != IR_OP_BITWISE_NOT &&
	    opcode != IR_OP_CONVERT)
	{
		error("IR builder", "opcode %d is not unary", opcode);
	}
	instruction = appendBuiltInstruction(builder, opcode, type);
	instruction->left = operand;
	return finishValue(builder, instruction);
}

IrValueId irBuilderEmitBinary(
    IrBuilder *builder, IrOpcode opcode, IrType type, IrValueId left, IrValueId right)
{
	IrInstruction *instruction;
	switch (opcode)
	{
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
		break;
	default:
		error("IR builder", "opcode %d is not binary", opcode);
	}
	instruction = appendBuiltInstruction(builder, opcode, type);
	instruction->left = left;
	instruction->right = right;
	return finishValue(builder, instruction);
}

IrValueId irBuilderEmitPointerOffset(IrBuilder *builder,
                                     IrOpcode opcode,
                                     IrType resultType,
                                     IrValueId pointer,
                                     IrValueId index,
                                     int elementSize)
{
	IrInstruction *instruction;
	if ((opcode != IR_OP_POINTER_ADD && opcode != IR_OP_POINTER_SUBTRACT &&
	     opcode != IR_OP_POINTER_DIFFERENCE) ||
	    elementSize <= 0)
	{
		error("IR builder", "invalid pointer operation");
	}
	instruction = appendBuiltInstruction(builder, opcode, resultType);
	instruction->left = pointer;
	instruction->right = index;
	instruction->offset = elementSize;
	return finishValue(builder, instruction);
}

IrValueId irBuilderEmitCompare(IrBuilder *builder,
                               IrCompareCondition condition,
                               IrValueId left,
                               IrValueId right)
{
	IrInstruction *instruction =
	    appendBuiltInstruction(builder, IR_OP_COMPARE, irTypeInteger(32U, 0, 4U));
	instruction->left = left;
	instruction->right = right;
	instruction->condition = condition;
	return finishValue(builder, instruction);
}

IrValueId irBuilderEmitLoad(IrBuilder *builder, IrType type, IrValueId address)
{
	IrInstruction *instruction = appendBuiltInstruction(builder, IR_OP_LOAD, type);
	instruction->left = address;
	return finishValue(builder, instruction);
}

void irBuilderEmitStore(IrBuilder *builder, IrType type, IrValueId address, IrValueId value)
{
	IrInstruction *instruction = appendBuiltInstruction(builder, IR_OP_STORE, type);
	instruction->left = address;
	instruction->right = value;
}

void irBuilderEmitZeroMemory(IrBuilder *builder, IrValueId address, size_t size)
{
	IrInstruction *instruction;
	if (size == 0U || size > (size_t)INT_MAX)
	{
		error("IR builder", "invalid zero-memory size");
	}
	instruction = appendBuiltInstruction(builder, IR_OP_ZERO_MEMORY, irTypeVoid());
	instruction->left = address;
	instruction->offset = (int)size;
}

IrValueId irBuilderEmitCall(IrBuilder *builder,
                            IrType type,
                            IrSymbolId symbol,
                            IrCallingConvention callingConvention,
                            const IrValueId *arguments,
                            int argumentCount)
{
	IrInstruction *instruction;
	if (symbol == IR_SYMBOL_NONE)
	{
		error("IR builder", "indirect calls are not represented by a direct-call instruction");
	}
	instruction = appendBuiltInstruction(builder, IR_OP_CALL, type);
	instruction->symbol = symbol;
	instruction->callingConvention = callingConvention;
	irSetCallArguments(instruction, arguments, argumentCount);
	if (type.kind == IR_TYPE_VOID)
	{
		return IR_VALUE_NONE;
	}
	return finishValue(builder, instruction);
}

IrValueId irBuilderEmitCallIndirect(IrBuilder *builder,
                                    IrType type,
                                    IrValueId callee,
                                    IrCallingConvention callingConvention,
                                    const IrValueId *arguments,
                                    int argumentCount)
{
	IrInstruction *instruction;
	if (callee == IR_VALUE_NONE)
	{
		error("IR builder", "indirect call requires a callee value");
	}
	instruction = appendBuiltInstruction(builder, IR_OP_CALL_INDIRECT, type);
	instruction->left = callee;
	instruction->callingConvention = callingConvention;
	irSetCallArguments(instruction, arguments, argumentCount);
	if (type.kind == IR_TYPE_VOID)
	{
		return IR_VALUE_NONE;
	}
	return finishValue(builder, instruction);
}

IrValueId irBuilderEmitVariadic(
    IrBuilder *builder, IrOpcode opcode, IrType type, IrValueId address, IrValueId source)
{
	IrInstruction *instruction = appendBuiltInstruction(builder, opcode, type);
	instruction->left = address;
	instruction->right = source;
	return type.kind == IR_TYPE_VOID ? IR_VALUE_NONE : finishValue(builder, instruction);
}

void irBuilderEmitBranch(IrBuilder *builder, IrBlockId destination)
{
	IrInstruction *instruction = appendBuiltInstruction(builder, IR_OP_BRANCH, irTypeVoid());
	instruction->trueBlock = destination;
}

void irBuilderEmitConditionalBranch(IrBuilder *builder,
                                    IrValueId condition,
                                    IrBlockId trueBlock,
                                    IrBlockId falseBlock)
{
	IrInstruction *instruction =
	    appendBuiltInstruction(builder, IR_OP_BRANCH_CONDITIONAL, irTypeVoid());
	instruction->left = condition;
	instruction->trueBlock = trueBlock;
	instruction->falseBlock = falseBlock;
}

void irBuilderEmitReturn(IrBuilder *builder, IrValueId value)
{
	IrInstruction *instruction =
	    appendBuiltInstruction(builder, IR_OP_RETURN, builder->function->returnType);
	if (builder->function->returnType.kind == IR_TYPE_VOID)
	{
		if (value != IR_VALUE_NONE)
		{
			error("IR builder", "void function cannot return a value");
		}
	}
	else if (value == IR_VALUE_NONE)
	{
		error("IR builder", "non-void function must return a value");
	}
	instruction->left = value;
}

void irSetCallArguments(IrInstruction *instruction, const IrValueId *arguments, int count)
{
	if ((instruction->opcode != IR_OP_CALL && instruction->opcode != IR_OP_CALL_INDIRECT) ||
	    count < 0 || (count > 0 && arguments == NULL))
	{
		error("IR", "invalid call argument list");
	}
	free(instruction->arguments);
	instruction->arguments = NULL;
	instruction->argumentCount = count;
	if (count > 0)
	{
		instruction->arguments = xalloc((size_t)count * sizeof(*instruction->arguments));
		memcpy(instruction->arguments, arguments, (size_t)count * sizeof(*instruction->arguments));
	}
}

static void
verifyValue(const unsigned char *definitions, int valueCount, IrValueId value, const char *role)
{
	if (value < 0 || value >= valueCount || definitions[value] == 0U)
	{
		error("IR verifier", "%s references undefined value %d", role, value);
	}
}

static void verifyDominatedValue(const unsigned char *definitions,
                                 const int *definitionBlocks,
                                 const int *definitionInstructions,
                                 const unsigned char *dominators,
                                 int blockCount,
                                 int valueCount,
                                 IrValueId value,
                                 int useBlock,
                                 int useInstruction,
                                 const char *role)
{
	verifyValue(definitions, valueCount, value, role);
	if (definitionBlocks[value] == useBlock)
	{
		if (definitionInstructions[value] >= useInstruction)
		{
			error("IR verifier", "%s uses value %d before its definition", role, value);
		}
	}
	else if (dominators[useBlock * blockCount + definitionBlocks[value]] == 0U)
	{
		error("IR verifier", "%s uses non-dominating value %d", role, value);
	}
}

static void markSuccessors(const IrFunction *function,
                           const IrInstruction *terminator,
                           unsigned char *reachable)
{
	if (terminator->opcode == IR_OP_BRANCH || terminator->opcode == IR_OP_BRANCH_CONDITIONAL)
	{
		reachable[terminator->trueBlock] = 1U;
	}
	if (terminator->opcode == IR_OP_BRANCH_CONDITIONAL)
	{
		reachable[terminator->falseBlock] = 1U;
	}
	(void)function;
}

static int blockBranchesTo(const IrBasicBlock *block, IrBlockId destination)
{
	const IrInstruction *terminator = &block->instructions[block->instructionCount - 1];
	return (terminator->opcode == IR_OP_BRANCH && terminator->trueBlock == destination) ||
	       (terminator->opcode == IR_OP_BRANCH_CONDITIONAL &&
	        (terminator->trueBlock == destination || terminator->falseBlock == destination));
}

static unsigned char *computeDominators(const IrFunction *function)
{
	int blockCount = function->blockCount;
	unsigned char *reachable = xalloc((size_t)blockCount);
	unsigned char *dominators = xalloc((size_t)blockCount * (size_t)blockCount);
	int changed;
	int block;
	reachable[0] = 1U;
	do
	{
		changed = 0;
		for (block = 0; block < blockCount; ++block)
		{
			unsigned char *before = xalloc((size_t)blockCount);
			memcpy(before, reachable, (size_t)blockCount);
			if (reachable[block] != 0U)
			{
				const IrBasicBlock *basicBlock = &function->blocks[block];
				markSuccessors(function,
				               &basicBlock->instructions[basicBlock->instructionCount - 1],
				               reachable);
			}
			if (memcmp(before, reachable, (size_t)blockCount) != 0)
			{
				changed = 1;
			}
			free(before);
		}
	} while (changed);
	for (block = 0; block < blockCount; ++block)
	{
		int candidate;
		for (candidate = 0; candidate < blockCount; ++candidate)
		{
			dominators[block * blockCount + candidate] =
			    (unsigned char)(block == 0               ? candidate == 0
			                    : reachable[block] != 0U ? 1
			                                             : candidate == block);
		}
	}
	do
	{
		changed = 0;
		for (block = 1; block < blockCount; ++block)
		{
			int predecessor;
			int candidate;
			int hasPredecessor = 0;
			if (reachable[block] == 0U)
			{
				continue;
			}
			for (candidate = 0; candidate < blockCount; ++candidate)
			{
				unsigned char value = 1U;
				for (predecessor = 0; predecessor < blockCount; ++predecessor)
				{
					if (reachable[predecessor] != 0U &&
					    blockBranchesTo(&function->blocks[predecessor], block))
					{
						hasPredecessor = 1;
						value =
						    (unsigned char)(value != 0U &&
						                    dominators[predecessor * blockCount + candidate] != 0U);
					}
				}
				if (candidate == block)
				{
					value = 1U;
				}
				if (dominators[block * blockCount + candidate] != value)
				{
					dominators[block * blockCount + candidate] = value;
					changed = 1;
				}
			}
			if (!hasPredecessor)
			{
				free(reachable);
				free(dominators);
				error("IR verifier", "reachable block %d has no predecessor", block);
			}
		}
	} while (changed);
	free(reachable);
	return dominators;
}

IrType *irCollectValueTypes(const IrFunction *function)
{
	IrType *types;
	int blockIndex;
	if (function->nextValue < 0)
	{
		error("IR", "negative value count");
	}
	types = xalloc(((size_t)function->nextValue + 1U) * sizeof(*types));
	for (blockIndex = 0; blockIndex < function->blockCount; ++blockIndex)
	{
		const IrBasicBlock *block = &function->blocks[blockIndex];
		int index;
		for (index = 0; index < block->instructionCount; ++index)
		{
			const IrInstruction *instruction = &block->instructions[index];
			if (instruction->result != IR_VALUE_NONE)
			{
				if (instruction->result < 0 || instruction->result >= function->nextValue)
				{
					error("IR", "result outside the value table");
				}
				types[instruction->result] = instruction->type;
			}
		}
	}
	return types;
}

static void verifyType(IrType type)
{
	int valid = type.kind == IR_TYPE_VOID ? type.bits == 0U
	            : (type.kind == IR_TYPE_INTEGER || type.kind == IR_TYPE_AGGREGATE)
	                ? (type.bits == 8U || type.bits == 16U || type.bits == 32U)
	            : type.kind == IR_TYPE_POINTER ? type.bits == 32U
	            : type.kind == IR_TYPE_FLOAT   ? (type.bits == 32U || type.bits == 64U)
	                                           : 0;
	if (!valid || (type.kind != IR_TYPE_VOID &&
	               (type.alignment == 0U || (type.alignment & (type.alignment - 1U)) != 0U)))
	{
		error("IR verifier",
		      "invalid type kind %u, width %u, alignment %u",
		      type.kind,
		      type.bits,
		      type.alignment);
	}
}

static void requireSameType(IrType expected,
                            IrType actual,
                            const IrInstruction *instruction,
                            const IrFunction *function)
{
	if (!irTypesEqual(expected, actual))
	{
		error("IR verifier",
		      "%s type mismatch in function %d at %s:%d (value %d; expected %u/%u/%u, actual "
		      "%u/%u/%u)",
		      opcodeName(instruction->opcode),
		      function->symbol,
		      instruction->sourceFile >= 0 ? mcc.srcFile[instruction->sourceFile] : "IR",
		      instruction->sourceLine,
		      instruction->result,
		      expected.kind,
		      expected.bits,
		      expected.isUnsigned,
		      actual.kind,
		      actual.bits,
		      actual.isUnsigned);
	}
}

static void verifyInstructionTypes(const IrFunction *function,
                                   const IrInstruction *instruction,
                                   const IrType *types)
{
	IrType left = instruction->left == IR_VALUE_NONE ? irTypeVoid() : types[instruction->left];
	IrType right = instruction->right == IR_VALUE_NONE ? irTypeVoid() : types[instruction->right];
	IrType result = instruction->type;
	IrOpcode opcode = instruction->opcode;
	verifyType(result);
	if (opcode == IR_OP_PARAMETER)
	{
		if (instruction->offset < 0 || instruction->offset >= function->parameterCount)
		{
			error("IR verifier", "parameter index outside the function signature");
		}
		requireSameType(
		    function->parameterTypes[instruction->offset], result, instruction, function);
	}
	else if (opcode == IR_OP_CONSTANT_FLOAT)
	{
		if (result.kind != IR_TYPE_FLOAT)
		{
			error("IR verifier", "floating constant has a non-floating type");
		}
	}
	else if (opcode == IR_OP_CONSTANT_INTEGER)
	{
		if (result.kind != IR_TYPE_INTEGER && result.kind != IR_TYPE_POINTER &&
		    !(result.kind == IR_TYPE_AGGREGATE && instruction->integer == 0U))
		{
			error("IR verifier", "integer constant has an invalid type");
		}
	}
	else if (opcode == IR_OP_ADDRESS_OF || opcode == IR_OP_LOCAL_ADDRESS)
	{
		if (result.kind != IR_TYPE_POINTER)
		{
			error("IR verifier", "address operation must produce a pointer");
		}
	}
	else if (opcode == IR_OP_LOAD || opcode == IR_OP_STORE || opcode == IR_OP_ZERO_MEMORY)
	{
		if (left.kind != IR_TYPE_POINTER)
		{
			error("IR verifier", "memory access requires a pointer");
		}
		if (opcode == IR_OP_STORE)
		{
			requireSameType(result, right, instruction, function);
		}
		if (opcode == IR_OP_ZERO_MEMORY && instruction->offset < 0)
		{
			error("IR verifier", "negative memory extent");
		}
	}
	else if (opcode == IR_OP_COPY || opcode == IR_OP_NEGATE || opcode == IR_OP_BITWISE_NOT)
	{
		requireSameType(result, left, instruction, function);
		if (opcode == IR_OP_BITWISE_NOT && result.kind != IR_TYPE_INTEGER)
		{
			error("IR verifier", "bitwise operation requires integers");
		}
	}
	else if (opcode >= IR_OP_ADD && opcode <= IR_OP_SHIFT_RIGHT_UNSIGNED)
	{
		requireSameType(result, left, instruction, function);
		requireSameType(result, right, instruction, function);
		if (result.kind != IR_TYPE_INTEGER &&
		    !(result.kind == IR_TYPE_FLOAT && opcode >= IR_OP_ADD && opcode <= IR_OP_DIVIDE_SIGNED))
		{
			error("IR verifier", "invalid arithmetic type");
		}
	}
	else if (opcode >= IR_OP_POINTER_ADD && opcode <= IR_OP_POINTER_DIFFERENCE)
	{
		int difference = opcode == IR_OP_POINTER_DIFFERENCE;
		if (left.kind != IR_TYPE_POINTER ||
		    right.kind != (difference ? IR_TYPE_POINTER : IR_TYPE_INTEGER) ||
		    result.kind != (difference ? IR_TYPE_INTEGER : IR_TYPE_POINTER) ||
		    instruction->offset <= 0)
		{
			error("IR verifier", "invalid pointer arithmetic");
		}
	}
	else if (opcode == IR_OP_COMPARE)
	{
		requireSameType(left, right, instruction, function);
		if (left.kind == IR_TYPE_VOID || result.kind != IR_TYPE_INTEGER || result.bits != 32U ||
		    instruction->condition < IR_COMPARE_EQUAL ||
		    instruction->condition > IR_COMPARE_GREATER_EQUAL_UNSIGNED)
		{
			error("IR verifier", "invalid comparison");
		}
	}
	else if (opcode == IR_OP_CONVERT)
	{
		if (left.kind == IR_TYPE_VOID || result.kind == IR_TYPE_VOID)
		{
			error("IR verifier", "conversion requires scalar values");
		}
	}
	else if (opcode == IR_OP_BRANCH_CONDITIONAL && left.kind != IR_TYPE_INTEGER)
	{
		error("IR verifier", "branch condition must be integral");
	}
	else if (opcode == IR_OP_RETURN)
	{
		requireSameType(function->returnType, result, instruction, function);
		if (result.kind != IR_TYPE_VOID)
		{
			requireSameType(result, left, instruction, function);
		}
	}
	else if (opcode == IR_OP_CALL_INDIRECT && left.kind != IR_TYPE_POINTER)
	{
		error("IR verifier", "indirect call requires a function pointer");
	}
	else if (opcode >= IR_OP_VA_START && opcode <= IR_OP_VA_END)
	{
		if (left.kind != IR_TYPE_POINTER ||
		    (opcode == IR_OP_VA_COPY && right.kind != IR_TYPE_POINTER))
		{
			error("IR verifier", "variadic state must be addressed by a pointer");
		}
		if (opcode == IR_OP_VA_START && !function->isVariadic)
		{
			error("IR verifier", "va_start outside a variadic function");
		}
		if ((opcode == IR_OP_VA_ARGUMENT) == (result.kind == IR_TYPE_VOID))
		{
			error("IR verifier", "invalid variadic result type");
		}
	}
}

static void verifyFunction(const IrFunction *function)
{
	unsigned char *definitions;
	int *definitionBlocks;
	int *definitionInstructions;
	unsigned char *dominators;
	int blockIndex;
	IrType *valueTypes;
	if (function->blockCount <= 0 || function->blockCount > function->blockCapacity ||
	    function->blocks == NULL || function->nextValue < 0 || function->parameterCount < 0 ||
	    (function->parameterCount > 0 && function->parameterTypes == NULL) ||
	    function->localCount < 0 || function->localCount > function->localCapacity ||
	    (function->localCount > 0 && function->locals == NULL))
	{
		error("IR verifier", "invalid function storage or counts");
	}
	verifyType(function->returnType);
	if (function->blockCount == 0)
	{
		error("IR verifier", "function %d has no basic blocks", function->symbol);
	}
	definitions = xalloc((size_t)function->nextValue + 1U);
	memset(definitions, 0, (size_t)function->nextValue + 1U);
	definitionBlocks = xalloc(((size_t)function->nextValue + 1U) * sizeof(*definitionBlocks));
	definitionInstructions =
	    xalloc(((size_t)function->nextValue + 1U) * sizeof(*definitionInstructions));
	for (blockIndex = 0; blockIndex < function->blockCount; ++blockIndex)
	{
		const IrBasicBlock *block = &function->blocks[blockIndex];
		int instructionIndex;
		if (block->id != blockIndex)
		{
			error("IR verifier", "non-canonical block identity");
		}
		if (block->instructionCount <= 0 || block->instructionCount > block->instructionCapacity ||
		    block->instructions == NULL ||
		    !isTerminator(block->instructions[block->instructionCount - 1].opcode))
		{
			free(definitions);
			error("IR verifier", "block %d has no terminator", block->id);
		}
		for (instructionIndex = 0; instructionIndex < block->instructionCount; ++instructionIndex)
		{
			const IrInstruction *instruction = &block->instructions[instructionIndex];
			(void)opcodeName(instruction->opcode);
			if ((instruction->left != IR_VALUE_NONE &&
			     (instruction->left < 0 || instruction->left >= function->nextValue)) ||
			    (instruction->right != IR_VALUE_NONE &&
			     (instruction->right < 0 || instruction->right >= function->nextValue)) ||
			    instruction->argumentCount < 0 ||
			    (instruction->argumentCount > 0 && instruction->arguments == NULL))
			{
				error("IR verifier", "invalid instruction operands or argument storage");
			}
			if (isTerminator(instruction->opcode) &&
			    instructionIndex + 1 != block->instructionCount)
			{
				error("IR verifier", "instructions follow a terminator");
			}
			if ((!producesValue(instruction->opcode) || instruction->type.kind == IR_TYPE_VOID) &&
			    instruction->result != IR_VALUE_NONE)
			{
				error("IR verifier", "non-value instruction defines a result");
			}
			if (producesValue(instruction->opcode) && instruction->type.kind != IR_TYPE_VOID)
			{
				if (instruction->result < 0 || instruction->result >= function->nextValue ||
				    definitions[instruction->result] != 0U)
				{
					free(definitions);
					error("IR verifier", "instruction has an invalid or duplicate result");
				}
				definitions[instruction->result] = 1U;
				definitionBlocks[instruction->result] = block->id;
				definitionInstructions[instruction->result] = instructionIndex;
			}
			if ((instruction->opcode == IR_OP_BRANCH ||
			     instruction->opcode == IR_OP_BRANCH_CONDITIONAL) &&
			    !blockExists(function, instruction->trueBlock))
			{
				free(definitionInstructions);
				free(definitionBlocks);
				free(definitions);
				error("IR verifier", "branch references missing block %d", instruction->trueBlock);
			}
			if (instruction->opcode == IR_OP_BRANCH_CONDITIONAL &&
			    !blockExists(function, instruction->falseBlock))
			{
				free(definitionInstructions);
				free(definitionBlocks);
				free(definitions);
				error("IR verifier", "branch references missing block %d", instruction->falseBlock);
			}
		}
	}
	valueTypes = irCollectValueTypes(function);
	dominators = computeDominators(function);
	for (blockIndex = 0; blockIndex < function->blockCount; ++blockIndex)
	{
		const IrBasicBlock *block = &function->blocks[blockIndex];
		int instructionIndex;
		for (instructionIndex = 0; instructionIndex < block->instructionCount; ++instructionIndex)
		{
			const IrInstruction *instruction = &block->instructions[instructionIndex];
			int argumentIndex;
			if (usesLeft(instruction->opcode) &&
			    !(instruction->opcode == IR_OP_RETURN && instruction->type.kind == IR_TYPE_VOID))
			{
				verifyDominatedValue(definitions,
				                     definitionBlocks,
				                     definitionInstructions,
				                     dominators,
				                     function->blockCount,
				                     function->nextValue,
				                     instruction->left,
				                     block->id,
				                     instructionIndex,
				                     "instruction");
			}
			if (usesRight(instruction->opcode))
			{
				verifyDominatedValue(definitions,
				                     definitionBlocks,
				                     definitionInstructions,
				                     dominators,
				                     function->blockCount,
				                     function->nextValue,
				                     instruction->right,
				                     block->id,
				                     instructionIndex,
				                     "instruction");
			}
			for (argumentIndex = 0; argumentIndex < instruction->argumentCount; ++argumentIndex)
			{
				verifyDominatedValue(definitions,
				                     definitionBlocks,
				                     definitionInstructions,
				                     dominators,
				                     function->blockCount,
				                     function->nextValue,
				                     instruction->arguments[argumentIndex],
				                     block->id,
				                     instructionIndex,
				                     "call");
			}
			if (instruction->opcode == IR_OP_LOCAL_ADDRESS &&
			    (instruction->local < 0 || instruction->local >= function->localCount))
			{
				free(definitions);
				error(
				    "IR verifier", "local address references missing local %d", instruction->local);
			}
			verifyInstructionTypes(function, instruction, valueTypes);
		}
	}
	free(valueTypes);
	free(dominators);
	free(definitionInstructions);
	free(definitionBlocks);
	free(definitions);
}

void irVerifyModule(const IrModule *module)
{
	int functionIndex;
	int globalIndex;
	if (module == NULL)
	{
		error("IR verifier", "module is null");
	}
	if (module->functionCount < 0 || module->functionCount > module->functionCapacity ||
	    (module->functionCount != 0 && module->functions == NULL) || module->globalCount < 0 ||
	    module->globalCount > module->globalCapacity ||
	    (module->globalCount != 0 && module->globals == NULL))
	{
		error("IR verifier", "invalid module storage or counts");
	}
	for (globalIndex = 0; globalIndex < module->globalCount; ++globalIndex)
	{
		const IrGlobal *global = &module->globals[globalIndex];
		int relocationIndex;
		if (global->alignment <= 0 || (global->alignment & (global->alignment - 1)) != 0 ||
		    global->initializerSize > global->zeroFillSize ||
		    (global->initializerSize != 0U && global->initializer == NULL) ||
		    global->relocationCount < 0 || global->relocationCount > global->relocationCapacity ||
		    (global->relocationCount != 0 && global->relocations == NULL))
		{
			error("IR verifier", "invalid global storage or alignment");
		}
		for (relocationIndex = 0; relocationIndex < global->relocationCount; ++relocationIndex)
		{
			size_t offset = global->relocations[relocationIndex].offset;
			int previous;
			if (offset > global->initializerSize || global->initializerSize - offset < 4U)
			{
				error("IR verifier", "global relocation exceeds initializer storage");
			}
			for (previous = 0; previous < relocationIndex; ++previous)
			{
				size_t other = global->relocations[previous].offset;
				if (offset < other + 4U && other < offset + 4U)
				{
					error("IR verifier", "overlapping global relocations");
				}
			}
		}
	}
	for (functionIndex = 0; functionIndex < module->functionCount; ++functionIndex)
	{
		verifyFunction(&module->functions[functionIndex]);
	}
}

static void dumpType(FILE *output, IrType type)
{
	if (type.kind == IR_TYPE_VOID)
	{
		fprintf(output, "void");
	}
	else if (type.kind == IR_TYPE_POINTER)
	{
		fprintf(output, "ptr%u", type.bits);
	}
	else if (type.kind == IR_TYPE_AGGREGATE)
	{
		fprintf(output, "aggregate%u", type.bits);
	}
	else
	{
		fprintf(output,
		        "%c%u",
		        type.kind == IR_TYPE_FLOAT ? 'f' : (type.isUnsigned ? 'u' : 'i'),
		        type.bits);
	}
}

void irDumpModule(const IrModule *module, FILE *output)
{
	int globalIndex;
	int functionIndex;
	if (output == NULL)
	{
		error("IR", "cannot dump IR to a null stream");
	}
	for (globalIndex = 0; globalIndex < module->globalCount; ++globalIndex)
	{
		const IrGlobal *global = &module->globals[globalIndex];
		fprintf(output, "global @%d : ", global->symbol);
		dumpType(output, global->type);
		fprintf(output,
		        " size=%u align=%d init=%u%s%s\n",
		        (unsigned int)global->zeroFillSize,
		        global->alignment,
		        (unsigned int)global->initializerSize,
		        global->isExternal ? " external" : "",
		        global->isExported ? " exported" : "");
		if (global->relocationCount > 0)
		{
			int relocationIndex;

			for (relocationIndex = 0; relocationIndex < global->relocationCount; ++relocationIndex)
			{
				const IrRelocation *relocation = &global->relocations[relocationIndex];

				fprintf(output,
				        "  relocate +%u -> @%d %+d\n",
				        (unsigned int)relocation->offset,
				        relocation->symbol,
				        relocation->addend);
			}
		}
	}
	for (functionIndex = 0; functionIndex < module->functionCount; ++functionIndex)
	{
		const IrFunction *function = &module->functions[functionIndex];
		int blockIndex;
		int parameterIndex;
		int localIndex;
		fprintf(output, "function @%d(", function->symbol);
		for (parameterIndex = 0; parameterIndex < function->parameterCount; ++parameterIndex)
		{
			if (parameterIndex > 0)
			{
				fprintf(output, ", ");
			}
			dumpType(output, function->parameterTypes[parameterIndex]);
		}
		if (function->isVariadic)
		{
			fprintf(output, "%s...", function->parameterCount > 0 ? ", " : "");
		}
		fprintf(output, ") -> ");
		dumpType(output, function->returnType);
		fprintf(output,
		        " cc=%d%s%s {\n",
		        function->callingConvention,
		        function->isInternal ? " internal" : "",
		        function->isExported ? " exported" : "");
		for (localIndex = 0; localIndex < function->localCount; ++localIndex)
		{
			const IrLocal *local = &function->locals[localIndex];
			fprintf(output, "  local %%%d symbol=%d type=", local->id, local->symbol);
			dumpType(output, local->type);
			fprintf(output,
			        " size=%u align=%d parameter=%d\n",
			        (unsigned int)local->size,
			        local->alignment,
			        local->parameterIndex);
		}
		for (blockIndex = 0; blockIndex < function->blockCount; ++blockIndex)
		{
			const IrBasicBlock *block = &function->blocks[blockIndex];
			int instructionIndex;
			fprintf(output, "block%d:\n", block->id);
			for (instructionIndex = 0; instructionIndex < block->instructionCount;
			     ++instructionIndex)
			{
				const IrInstruction *instruction = &block->instructions[instructionIndex];
				if (instruction->result != IR_VALUE_NONE)
				{
					fprintf(output, "  %%%d = ", instruction->result);
				}
				else
				{
					fprintf(output, "  ");
				}
				fprintf(output, "%s ", opcodeName(instruction->opcode));
				dumpType(output, instruction->type);
				fprintf(output,
				        " %d %d integer=%u float=%.17g offset=%d symbol=%d local=%d blocks=%d,%d "
				        "condition=%d cc=%d",
				        instruction->left,
				        instruction->right,
				        (unsigned int)instruction->integer,
				        instruction->floating,
				        instruction->offset,
				        instruction->symbol,
				        instruction->local,
				        instruction->trueBlock,
				        instruction->falseBlock,
				        instruction->condition,
				        instruction->callingConvention);
				if (instruction->argumentCount > 0)
				{
					int argumentIndex;
					fprintf(output, " args=(");
					for (argumentIndex = 0; argumentIndex < instruction->argumentCount;
					     ++argumentIndex)
					{
						fprintf(output,
						        "%s%%%d",
						        argumentIndex > 0 ? "," : "",
						        instruction->arguments[argumentIndex]);
					}
					fprintf(output, ")");
				}
				fprintf(output, "\n");
			}
		}
		fprintf(output, "}\n");
	}
}

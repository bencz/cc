/* HLASM lowering for the typed CFG IR using z/OS LE non-XPLINK linkage. */

#include "cc.h"

#include <stdarg.h>

#define HLASM_LINE_LIMIT 71U
#define MAIN_ARGUMENT_BYTES 3071
#define MAIN_ARGUMENT_COUNT 256

typedef struct _HLASM_FUNCTION
{
	int id;
	int ordinal;
	unsigned int blockBase;
	int frameSize;
	int maxStack;
	int exported;
	int isMain;
	char entry[9];
	char ppa[9];
	char dsect[9];
	char params[9];
	char locals[9];
	char argList[9];
	char workSize[9];
	char mainArgc[9];
	char mainArgv[9];
	char mainPlist[9];
	char mainBuffer[9];
	char mainArgvList[9];
	const IrFunction *ir;
	int *localOffsets;
	int *valueOffsets;
	IrType *valueTypes;
} HLASM_FUNCTION;

typedef struct _HLASM_STATE
{
	FILE *output;
	HLASM_FUNCTION *functions;
	int functionCount;
	HLASM_FUNCTION *function;
	unsigned int generatedLabel;
	char unitName[24];
} HLASM_STATE;

static HLASM_STATE hs;

/* IBM-1047 for the invariant ASCII range.  IBM-037 differs in three graphic
 * positions; literals are emitted as hexadecimal bytes, independent of the
 * transfer encoding of the assembler source data set. */
static const unsigned char ibm1047_ascii[128] = {
    0x00, 0x01, 0x02, 0x03, 0x37, 0x2D, 0x2E, 0x2F, 0x16, 0x05, 0x15, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x3C, 0x3D, 0x32, 0x26, 0x18, 0x19, 0x3F, 0x27, 0x1C, 0x1D, 0x1E, 0x1F,
    0x40, 0x5A, 0x7F, 0x7B, 0x5B, 0x6C, 0x50, 0x7D, 0x4D, 0x5D, 0x5C, 0x4E, 0x6B, 0x60, 0x4B, 0x61,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0x7A, 0x5E, 0x4C, 0x7E, 0x6E, 0x6F,
    0x7C, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6,
    0xD7, 0xD8, 0xD9, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xAD, 0xE0, 0xBD, 0x5F, 0x6D,
    0x79, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96,
    0x97, 0x98, 0x99, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xC0, 0x4F, 0xD0, 0xA1, 0x07};

static void emit(const char *format, ...)
{
	char line[512];
	va_list arguments;
	int length;

	va_start(arguments, format);
	length = vsnprintf(line, sizeof(line), format, arguments);
	va_end(arguments);
	if (length < 0 || (size_t)length >= sizeof(line))
	{
		error("hlasm.emit", "assembler line formatting overflow");
	}
	while (length > 0 && (line[length - 1] == ' ' || line[length - 1] == '\t'))
	{
		line[--length] = '\0';
	}
	if ((size_t)length > HLASM_LINE_LIMIT)
	{
		error("hlasm.emit", "assembler statement exceeds column 71: %s", line);
	}
	if (fprintf(hs.output, "%s\n", line) < 0)
	{
		error("hlasm.emit", "write error");
	}
}

static void emit_op(const char *label, const char *operation, const char *format, ...)
{
	char operand[384];
	va_list arguments;
	int length;

	operand[0] = '\0';
	if (format != NULL && *format != '\0')
	{
		va_start(arguments, format);
		length = vsnprintf(operand, sizeof(operand), format, arguments);
		va_end(arguments);
		if (length < 0 || (size_t)length >= sizeof(operand))
		{
			error("hlasm.emit", "assembler operand formatting overflow");
		}
	}
	/* Operand continuation uses column 72 and resumes in column 16. */
	if (strlen(operand) > HLASM_LINE_LIMIT - 18U)
	{
		size_t offset = HLASM_LINE_LIMIT - 18U;
		if (fprintf(hs.output,
		            "%-8s %-8s %.*sX\n",
		            label != NULL ? label : "",
		            operation,
		            (int)offset,
		            operand) < 0)
		{
			error("hlasm.emit", "write error");
		}
		while (strlen(operand + offset) > HLASM_LINE_LIMIT - 15U)
		{
			if (fprintf(
			        hs.output, "%15s%.*sX\n", "", (int)(HLASM_LINE_LIMIT - 15U), operand + offset) <
			    0)
			{
				error("hlasm.emit", "write error");
			}
			offset += HLASM_LINE_LIMIT - 15U;
		}
		emit("%15s%s", "", operand + offset);
		return;
	}
	/* RIL branches cover large generated functions without branch relaxation. */
	if (operation[0] == 'J')
	{
		char longBranch[12];
		(void)snprintf(longBranch, sizeof(longBranch), "JL%s", operation + 1);
		emit("%-8s %-8s %s", label != NULL ? label : "", longBranch, operand);
	}
	else
	{
		emit("%-8s %-8s %s", label != NULL ? label : "", operation, operand);
	}
}

static void emit_alias(const char *symbol, const char *externalName)
{
	char prefix[32];
	size_t prefixLength;
	size_t nameLength = strlen(externalName);
	int length = snprintf(prefix, sizeof(prefix), "%-8s %-8s C'", symbol, "ALIAS");
	if (length < 0 || (size_t)length >= sizeof(prefix))
	{
		error("hlasm.emit", "ALIAS prefix formatting overflow");
	}
	prefixLength = (size_t)length;
	if (prefixLength + nameLength + 1U <= HLASM_LINE_LIMIT)
	{
		emit("%s%s'", prefix, externalName);
		return;
	}
	/* Column 72 is the continuation indicator and column 16 is the standard
	 * continuation column.  Identifier characters can be split safely inside
	 * the quoted ALIAS operand. */
	{
		size_t first = HLASM_LINE_LIMIT - prefixLength;
		if (fprintf(hs.output,
		            "%s%.*sX\n%15s%s'\n",
		            prefix,
		            (int)first,
		            externalName,
		            "",
		            externalName + first) < 0)
		{
			error("hlasm.emit", "write error");
		}
	}
}

static const char *source_name(int id)
{
	if (id < 0 || id >= cd.hash.size || cd.hash.tbl[id].state != 1U || cd.hash.tbl[id].key == NULL)
	{
		error("hlasm", "invalid symbol id %d", id);
	}
	return cd.hash.tbl[id].key;
}

static void make_name(char output[9], char prefix, unsigned int value)
{
	if (value > 0xFFFFFFU)
	{
		error("hlasm", "assembler symbol space exhausted");
	}
	(void)snprintf(output, 9U, "%c%06X", prefix, value);
}

static void symbol_name(char output[9], int id)
{
	make_name(output, 'F', (unsigned int)id);
}

static void ir_symbol_name(char output[9], IrSymbolId symbol)
{
	if (symbol >= 0)
	{
		symbol_name(output, symbol);
	}
	else
	{
		make_name(output, 'K', (unsigned int)(-symbol));
	}
}

static void location_name(char output[9], int location)
{
	make_name(output, 'L', hs.function->blockBase + (unsigned int)location);
}

static void generated_name(char output[9])
{
	make_name(output, 'G', hs.generatedLabel++);
}

static HLASM_FUNCTION *find_function(int id)
{
	int index;
	for (index = 0; index < hs.functionCount; ++index)
	{
		if (hs.functions[index].id == id)
		{
			return &hs.functions[index];
		}
	}
	return NULL;
}

static int is_defined_function(int id)
{
	return find_function(id) != NULL;
}

static int symbol_was_emitted(const int *symbols, int count, int id)
{
	int index;
	for (index = 0; index < count; ++index)
	{
		if (symbols[index] == id)
		{
			return 1;
		}
	}
	return 0;
}

static int ir_type_size(IrType type)
{
	if (type.kind == IR_TYPE_VOID)
	{
		return 0;
	}
	if (type.bits == 0U || (type.bits & 7U) != 0U)
	{
		error("hlasm IR", "invalid type width %u", type.bits);
	}
	return type.bits / CHAR_BIT;
}

static int align_up(int value, int alignment)
{
	int remainder;
	if (alignment <= 0 || value < 0 || value > INT_MAX - alignment)
	{
		error("hlasm IR", "invalid alignment %d", alignment);
	}
	remainder = value % alignment;
	return remainder == 0 ? value : value + alignment - remainder;
}

static void initialize_function_names(HLASM_FUNCTION *function, int ordinal, int symbol)
{
	function->id = symbol;
	function->ordinal = ordinal;
	function->isMain = symbol >= 0 && strcmp(source_name(symbol), "_main") == 0;
	ir_symbol_name(function->entry, symbol);
	make_name(function->ppa, 'P', (unsigned int)ordinal);
	make_name(function->dsect, 'D', (unsigned int)ordinal);
	make_name(function->params, 'Q', (unsigned int)ordinal);
	make_name(function->locals, 'A', (unsigned int)ordinal);
	make_name(function->argList, 'C', (unsigned int)ordinal);
	make_name(function->workSize, 'W', (unsigned int)ordinal);
	make_name(function->mainArgc, 'N', (unsigned int)ordinal);
	make_name(function->mainArgv, 'V', (unsigned int)ordinal);
	make_name(function->mainPlist, 'T', (unsigned int)ordinal);
	make_name(function->mainBuffer, 'U', (unsigned int)ordinal);
	make_name(function->mainArgvList, 'X', (unsigned int)ordinal);
}

static void analyze_ir_functions(void)
{
	int functionIndex;
	unsigned int blockBase = 0U;
	hs.functionCount = compiler.ir.functionCount;
	hs.functions = xalloc((size_t)hs.functionCount * sizeof(*hs.functions));
	for (functionIndex = 0; functionIndex < hs.functionCount; ++functionIndex)
	{
		const IrFunction *irFunction = &compiler.ir.functions[functionIndex];
		HLASM_FUNCTION *function = &hs.functions[functionIndex];
		int offset = 0;
		int localIndex;
		int blockIndex;
		initialize_function_names(function, functionIndex, irFunction->symbol);
		function->blockBase = blockBase;
		blockBase += (unsigned int)irFunction->blockCount;
		if (blockBase > 0xFFFFFFU)
		{
			error("hlasm", "too many control-flow blocks");
		}
		function->ir = irFunction;
		function->exported = irFunction->isExported;
		function->localOffsets =
		    xalloc((size_t)irFunction->localCount * sizeof(*function->localOffsets));
		function->valueOffsets =
		    xalloc((size_t)irFunction->nextValue * sizeof(*function->valueOffsets));
		function->valueTypes =
		    xalloc((size_t)irFunction->nextValue * sizeof(*function->valueTypes));
		for (localIndex = 0; localIndex < irFunction->localCount; ++localIndex)
		{
			const IrLocal *local = &irFunction->locals[localIndex];
			offset = align_up(offset, local->alignment);
			function->localOffsets[local->id] = offset;
			if (local->size > (size_t)(INT_MAX - offset - 65536))
			{
				error("hlasm IR", "local frame exceeds the 31-bit address space");
			}
			offset += (int)local->size;
		}
		for (blockIndex = 0; blockIndex < irFunction->blockCount; ++blockIndex)
		{
			const IrBasicBlock *block = &irFunction->blocks[blockIndex];
			int instructionIndex;
			for (instructionIndex = 0; instructionIndex < block->instructionCount;
			     ++instructionIndex)
			{
				const IrInstruction *instruction = &block->instructions[instructionIndex];
				int argumentBytes = instruction->argumentCount * 8 + 4;
				if (instruction->result != IR_VALUE_NONE)
				{
					int size = ir_type_size(instruction->type);
					int alignment = instruction->type.alignment;
					offset = align_up(offset, alignment);
					function->valueOffsets[instruction->result] = offset;
					function->valueTypes[instruction->result] = instruction->type;
					offset += size;
				}
				if (argumentBytes > function->maxStack)
				{
					function->maxStack = argumentBytes;
				}
			}
		}
		function->frameSize = align_up(offset, 8);
		if (function->maxStack < 8)
		{
			function->maxStack = 8;
		}
	}
}

unsigned char hlasmExecutionByte(unsigned char ascii)
{
	if (ascii >= 128U)
	{
		error("hlasm.charset", "non-ASCII source byte 0x%02X requires a universal escape", ascii);
	}
	if (cmd.executionCharset == EXEC_CHARSET_IBM037)
	{
		if (ascii == '[')
		{
			return 0xBA;
		}
		if (ascii == ']')
		{
			return 0xBB;
		}
		if (ascii == '^')
		{
			return 0xB0;
		}
	}
	return ibm1047_ascii[ascii];
}

static void load_immediate(int reg, int value)
{
	if (value >= -32768 && value <= 32767)
	{
		emit_op(NULL, "LHI", "%d,%d", reg, value);
	}
	else
	{
		emit_op(NULL, "LGFI", "%d,%d", reg, value);
	}
}

static void add_immediate(int reg, int value)
{
	if (value >= -32768 && value <= 32767)
	{
		emit_op(NULL, "AHI", "%d,%d", reg, value);
	}
	else
	{
		emit_op(NULL, "AFI", "%d,%d", reg, value);
	}
}

static void address_from_base(int reg, const char *base, int offset)
{
	emit_op(NULL, "LR", "%d,13", reg);
	emit_op(NULL, "AFI", "%d,%s-%s+%d", reg, base, hs.function->dsect, offset);
}

static void set_condition_result(const char *branch)
{
	char yes[9], done[9];
	generated_name(yes);
	generated_name(done);
	emit_op(NULL, branch, "%s", yes);
	emit_op(NULL, "XR", "2,2");
	emit_op(NULL, "J", "%s", done);
	emit_op(yes, "DS", "0H");
	emit_op(NULL, "LHI", "2,1");
	emit_op(done, "DS", "0H");
}

static void dsa_address(int reg, const char *field)
{
	emit_op(NULL, "LAY", "%d,%s(,13)", reg, field);
}

/* Convert the LE PLIST(HOST) halfword-prefixed parameter string into the
 * argc/argv representation expected by C main.  The parser operates on the
 * native EBCDIC bytes and supports whitespace plus double-quoted arguments.
 * Explicit bounds keep malformed or hostile invocation data inside the DSA. */
static void emit_main_arguments_and_call(int symbolId)
{
	char noParameters[9], skipSpaces[9], startArgument[9], copyArgument[9];
	char toggleQuote[9], storeCharacter[9], advanceSource[9];
	char endArgument[9], finish[9], overflow[9], callDone[9];
	char symbol[9];
	generated_name(noParameters);
	generated_name(skipSpaces);
	generated_name(startArgument);
	generated_name(copyArgument);
	generated_name(toggleQuote);
	generated_name(storeCharacter);
	generated_name(advanceSource);
	generated_name(endArgument);
	generated_name(finish);
	generated_name(overflow);
	generated_name(callDone);

	dsa_address(8, hs.function->mainBuffer);
	emit_op(NULL, "MVI", "0(8),X'00'");
	dsa_address(9, hs.function->mainArgvList);
	emit_op(NULL, "ST", "8,0(,9)");
	emit_op(NULL, "LHI", "10,1");
	emit_op(NULL, "L", "6,%s(,13)", hs.function->params);
	emit_op(NULL, "LTR", "6,6");
	emit_op(NULL, "JZ", "%s", noParameters);
	emit_op(NULL, "L", "6,0(,6)");
	emit_op(NULL, "NILF", "6,X'7FFFFFFF'");
	emit_op(NULL, "LTR", "6,6");
	emit_op(NULL, "JZ", "%s", noParameters);
	emit_op(NULL, "LLH", "7,0(,6)");
	emit_op(NULL, "CHI", "7,%d", MAIN_ARGUMENT_BYTES);
	emit_op(NULL, "JH", "%s", overflow);
	emit_op(NULL, "LA", "6,2(,6)");
	emit_op(NULL, "LA", "8,1(,8)");
	emit_op(NULL, "J", "%s", skipSpaces);

	emit_op(noParameters, "XR", "7,7");
	emit_op(NULL, "LA", "8,1(,8)");
	emit_op(NULL, "J", "%s", finish);
	emit_op(skipSpaces, "LTR", "7,7");
	emit_op(NULL, "JZ", "%s", finish);
	emit_op(NULL, "CLI", "0(6),X'40'");
	emit_op(NULL, "JNE", "%s", startArgument);
	emit_op(NULL, "LA", "6,1(,6)");
	emit_op(NULL, "AHI", "7,-1");
	emit_op(NULL, "J", "%s", skipSpaces);

	emit_op(startArgument, "CHI", "10,%d", MAIN_ARGUMENT_COUNT);
	emit_op(NULL, "JNL", "%s", overflow);
	emit_op(NULL, "LR", "1,10");
	emit_op(NULL, "SLL", "1,2");
	emit_op(NULL, "LA", "1,0(1,9)");
	emit_op(NULL, "ST", "8,0(,1)");
	emit_op(NULL, "AHI", "10,1");
	emit_op(NULL, "XR", "5,5");

	emit_op(copyArgument, "LTR", "7,7");
	emit_op(NULL, "JZ", "%s", endArgument);
	emit_op(NULL, "XR", "0,0");
	emit_op(NULL, "IC", "0,0(,6)");
	emit_op(NULL, "CHI", "0,127");
	emit_op(NULL, "JE", "%s", toggleQuote);
	emit_op(NULL, "CHI", "0,64");
	emit_op(NULL, "JNE", "%s", storeCharacter);
	emit_op(NULL, "LTR", "5,5");
	emit_op(NULL, "JZ", "%s", endArgument);
	emit_op(NULL, "J", "%s", storeCharacter);
	emit_op(toggleQuote, "XILF", "5,1");
	emit_op(NULL, "J", "%s", advanceSource);
	emit_op(storeCharacter, "STC", "0,0(,8)");
	emit_op(NULL, "LA", "8,1(,8)");
	emit_op(advanceSource, "LA", "6,1(,6)");
	emit_op(NULL, "AHI", "7,-1");
	emit_op(NULL, "J", "%s", copyArgument);

	emit_op(endArgument, "MVI", "0(8),X'00'");
	emit_op(NULL, "LA", "8,1(,8)");
	emit_op(NULL, "J", "%s", skipSpaces);

	emit_op(finish, "LR", "1,10");
	emit_op(NULL, "SLL", "1,2");
	emit_op(NULL, "LA", "1,0(1,9)");
	emit_op(NULL, "XR", "0,0");
	emit_op(NULL, "ST", "0,0(,1)");
	dsa_address(1, hs.function->mainArgc);
	emit_op(NULL, "ST", "10,0(,1)");
	dsa_address(1, hs.function->mainArgv);
	emit_op(NULL, "ST", "9,0(,1)");
	dsa_address(1, hs.function->mainPlist);
	emit_op(NULL, "ST", "10,0(,1)");
	emit_op(NULL, "ST", "9,4(,1)");
	symbol_name(symbol, symbolId);
	emit_op(NULL, "LARL", "15,%s", symbol);
	emit_op(NULL, "BALR", "14,15");
	emit_op(NULL, "J", "%s", callDone);

	emit_op(overflow, "LHI", "15,2");
	emit_op(callDone, "LR", "2,15");
}

static void emit_function_entry(HLASM_FUNCTION *function)
{
	emit("*");
	if (function->ordinal > 0)
	{
		emit_op(NULL, "DROP", "11,13");
	}
	if (function->isMain)
	{
		emit_op(function->entry,
		        "CEEENTRY",
		        "PPA=%s,AUTO=%s,MAIN=YES,EXECOPS=NO,AMODE=31",
		        function->ppa,
		        function->workSize);
	}
	else if (function->exported)
	{
		emit_op(function->entry,
		        "CEEENTRY",
		        "PPA=%s,AUTO=%s,MAIN=NO,EXPORT=YES,AMODE=31",
		        function->ppa,
		        function->workSize);
	}
	else
	{
		emit_op(function->entry,
		        "CEEENTRY",
		        "PPA=%s,AUTO=%s,MAIN=NO,AMODE=31",
		        function->ppa,
		        function->workSize);
	}
	emit_op(NULL, "USING", "%s,13", function->dsect);
	emit_op(NULL, "ST", "1,%s(,13)", function->params);
	emit_op(NULL, "J", "E%06X", (unsigned int)function->ordinal);
	emit_op(NULL, "LTORG", "");
	{
		char body[9];
		make_name(body, 'E', (unsigned int)function->ordinal);
		emit_op(body, "DS", "0H");
	}
	hs.function = function;
}

static void emit_ir_bytes(const unsigned char *bytes, size_t begin, size_t end)
{
	while (begin < end)
	{
		char encoded[49];
		size_t count = end - begin;
		size_t index;
		if (count > 24U)
		{
			count = 24U;
		}
		for (index = 0; index < count; ++index)
		{
			(void)snprintf(
			    encoded + index * 2U, sizeof(encoded) - index * 2U, "%02X", bytes[begin + index]);
		}
		emit_op(NULL, "DC", "X'%s'", encoded);
		begin += count;
	}
}

static void emit_zero_bytes(size_t size)
{
	while (size != 0U)
	{
		unsigned int count = size > 65535U ? 65535U : (unsigned int)size;
		emit_op(NULL, "DC", "%uX'00'", count);
		size -= count;
	}
}

static void emit_unit_alias(const char *symbol, const char *kind, int ordinal)
{
	char name[64];
	(void)snprintf(name, sizeof(name), "__cc_%s_%s_%u", hs.unitName, kind, (unsigned int)ordinal);
	emit_alias(symbol, name);
}

static void emit_ir_data_objects(void)
{
	int globalIndex;
	for (globalIndex = 0; globalIndex < compiler.ir.globalCount; ++globalIndex)
	{
		const IrGlobal *global = &compiler.ir.globals[globalIndex];
		char label[9];
		size_t offset = 0U;
		int relocationIndex;
		if (global->isExternal)
		{
			continue;
		}
		if (global->alignment >= 8)
		{
			emit_op(NULL, "DS", "0D");
		}
		else if (global->alignment >= 4)
		{
			emit_op(NULL, "DS", "0F");
		}
		else
		{
			emit_op(NULL, "DS", "0H");
		}
		ir_symbol_name(label, global->symbol);
		if (global->symbol >= 0 && !global->isInternal)
		{
			emit_op(NULL, "ENTRY", "%s", label);
			emit_alias(label, source_name(global->symbol));
		}
		emit_op(label, "DS", "0C");
		if (global->initializer == NULL)
		{
			emit_zero_bytes(global->zeroFillSize);
			continue;
		}
		for (relocationIndex = 0; relocationIndex < global->relocationCount; ++relocationIndex)
		{
			const IrRelocation *relocation = &global->relocations[relocationIndex];
			char target[9];
			emit_ir_bytes(global->initializer, offset, relocation->offset);
			ir_symbol_name(target, relocation->symbol);
			if (relocation->addend > 0)
			{
				emit_op(NULL, "DC", "A(%s+%d)", target, relocation->addend);
			}
			else if (relocation->addend < 0)
			{
				emit_op(NULL, "DC", "A(%s%d)", target, relocation->addend);
			}
			else
			{
				emit_op(NULL, "DC", "A(%s)", target);
			}
			offset = relocation->offset + 4U;
		}
		emit_ir_bytes(global->initializer, offset, global->initializerSize);
		if (global->initializerSize < global->zeroFillSize)
		{
			emit_zero_bytes(global->zeroFillSize - global->initializerSize);
		}
	}
}

static void emit_data(void)
{
	emit("*");
	emit_op("CCDATA", "CSECT", "");
	emit_unit_alias("CCDATA", "data", 0);
	emit_op("CCDATA", "AMODE", "31");
	emit_op("CCDATA", "RMODE", "ANY");
	emit_op(NULL, "DS", "0D");
	emit_ir_data_objects();
	emit_op("CCPROG", "RSECT", "");
	emit_unit_alias("CCPROG", "code", 0);
}

static void emit_dsects(void)
{
	int index;
	if (hs.functionCount == 0)
	{
		emit_op("CCPROG", "RSECT", "");
		return;
	}
	for (index = 0; index < hs.functionCount; ++index)
	{
		emit_op(hs.functions[index].ppa,
		        "CEEPPA",
		        index == 0 ? "EPNAME=%s" : "EPNAME=%s,PPA2=NO",
		        hs.functions[index].entry);
	}
	for (index = 0; index < hs.functionCount; ++index)
	{
		HLASM_FUNCTION *function = &hs.functions[index];
		emit("*");
		emit_op(function->dsect, "DSECT", "");
		emit_op(NULL, "ORG", "%s+CEEDSASZ", function->dsect);
		emit_op(function->params, "DS", "A");
		emit_op(NULL, "DS", "0D");
		emit_op(function->locals, "DS", "XL%d", function->frameSize > 0 ? function->frameSize : 1);
		emit_op(function->argList, "DS", "%dA", function->maxStack / 4 + 1);
		if (function->isMain)
		{
			emit_op(function->mainArgc, "DS", "F");
			emit_op(function->mainArgv, "DS", "A");
			emit_op(function->mainPlist, "DS", "2A");
			emit_op(function->mainBuffer, "DS", "XL%d", MAIN_ARGUMENT_BYTES + 2);
			emit_op(NULL, "DS", "0F");
			emit_op(function->mainArgvList, "DS", "%dA", MAIN_ARGUMENT_COUNT + 1);
		}
		emit_op(function->workSize, "EQU", "*-%.8s", function->dsect);
	}
	emit_op(NULL, "CEEDSA", "");
	emit_op(NULL, "CEECAA", "");
	emit_op("CCPROG", "RSECT", "");
}

static int ir_value_offset(IrValueId value)
{
	if (hs.function == NULL || hs.function->ir == NULL || value < 0 ||
	    value >= hs.function->ir->nextValue)
	{
		error("hlasm IR", "invalid value %d", value);
	}
	return hs.function->valueOffsets[value];
}

static IrType ir_value_type(IrValueId value)
{
	(void)ir_value_offset(value);
	return hs.function->valueTypes[value];
}

static void load_ir_integer(int reg, IrValueId value)
{
	IrType type = ir_value_type(value);
	int offset = ir_value_offset(value);
	if (type.kind == IR_TYPE_FLOAT || type.kind == IR_TYPE_VOID)
	{
		error("hlasm IR", "value %d is not integral", value);
	}
	address_from_base(1, hs.function->locals, offset);
	if (type.bits == 32U)
	{
		emit_op(NULL, "L", "%d,0(,1)", reg);
	}
	else if (type.bits == 16U)
	{
		emit_op(NULL, type.isUnsigned ? "LLH" : "LH", "%d,0(,1)", reg);
	}
	else if (type.bits == 8U)
	{
		emit_op(NULL, "LLGC", "%d,0(,1)", reg);
		if (!type.isUnsigned)
		{
			emit_op(NULL, "SLL", "%d,24", reg);
			emit_op(NULL, "SRA", "%d,24", reg);
		}
	}
	else
	{
		error("hlasm IR", "unsupported integral width %u", type.bits);
	}
}

static void store_ir_integer(int reg, IrValueId value)
{
	IrType type = ir_value_type(value);
	int offset = ir_value_offset(value);
	const char *operation;
	if (type.kind == IR_TYPE_FLOAT || type.kind == IR_TYPE_VOID)
	{
		error("hlasm IR", "value %d is not integral", value);
	}
	operation = type.bits == 32U ? "ST" : type.bits == 16U ? "STH" : type.bits == 8U ? "STC" : NULL;
	if (operation == NULL)
	{
		error("hlasm IR", "unsupported integral width %u", type.bits);
	}
	address_from_base(1, hs.function->locals, offset);
	emit_op(NULL, operation, "%d,0(,1)", reg);
}

static void load_ir_float(int reg, IrValueId value)
{
	IrType type = ir_value_type(value);
	if (type.kind != IR_TYPE_FLOAT || (type.bits != 32U && type.bits != 64U))
	{
		error("hlasm IR", "only IEEE binary64 values are supported");
	}
	address_from_base(1, hs.function->locals, ir_value_offset(value));
	emit_op(NULL, type.bits == 32U ? "LE" : "LD", "%d,0(,1)", reg);
	if (type.bits == 32U)
	{
		emit_op(NULL, "LDEBR", "%d,%d", reg, reg);
	}
}

static void store_ir_float(int reg, IrValueId value)
{
	IrType type = ir_value_type(value);
	if (type.kind != IR_TYPE_FLOAT || (type.bits != 32U && type.bits != 64U))
	{
		error("hlasm IR", "only IEEE binary64 values are supported");
	}
	address_from_base(1, hs.function->locals, ir_value_offset(value));
	if (type.bits == 32U)
	{
		emit_op(NULL, "LEDBR", "%d,%d", reg, reg);
	}
	emit_op(NULL, type.bits == 32U ? "STE" : "STD", "%d,0(,1)", reg);
}

static void emit_ir_block_label(IrBlockId block)
{
	char label[9];
	location_name(label, block);
	emit_op(label, "DS", "0H");
}

static const char *ir_comparison_branch(IrCompareCondition condition)
{
	switch (condition)
	{
	case IR_COMPARE_EQUAL:
		return "JE";
	case IR_COMPARE_NOT_EQUAL:
		return "JNE";
	case IR_COMPARE_LESS_SIGNED:
	case IR_COMPARE_LESS_UNSIGNED:
		return "JL";
	case IR_COMPARE_LESS_EQUAL_SIGNED:
	case IR_COMPARE_LESS_EQUAL_UNSIGNED:
		return "JNH";
	case IR_COMPARE_GREATER_SIGNED:
	case IR_COMPARE_GREATER_UNSIGNED:
		return "JH";
	case IR_COMPARE_GREATER_EQUAL_SIGNED:
	case IR_COMPARE_GREATER_EQUAL_UNSIGNED:
		return "JNL";
	default:
		error("hlasm IR", "invalid comparison condition %d", condition);
	}
}

static int ir_comparison_is_unsigned(IrCompareCondition condition)
{
	return condition == IR_COMPARE_LESS_UNSIGNED || condition == IR_COMPARE_LESS_EQUAL_UNSIGNED ||
	       condition == IR_COMPARE_GREATER_UNSIGNED ||
	       condition == IR_COMPARE_GREATER_EQUAL_UNSIGNED;
}

static void normalize_ir_integer(int reg, IrType type)
{
	if (type.bits == 8U)
	{
		emit_op(NULL, "NILF", "%d,X'000000FF'", reg);
		if (!type.isUnsigned)
		{
			emit_op(NULL, "SLL", "%d,24", reg);
			emit_op(NULL, "SRA", "%d,24", reg);
		}
	}
	else if (type.bits == 16U)
	{
		emit_op(NULL, "NILF", "%d,X'0000FFFF'", reg);
		if (!type.isUnsigned)
		{
			emit_op(NULL, "SLL", "%d,16", reg);
			emit_op(NULL, "SRA", "%d,16", reg);
		}
	}
	else if (type.bits != 32U)
	{
		error("hlasm IR", "unsupported integer conversion width %u", type.bits);
	}
}

static int c_parameter_offset(int count)
{
	int offset = (hs.function->ir->returnType.kind == IR_TYPE_FLOAT ||
	              hs.function->ir->returnType.kind == IR_TYPE_AGGREGATE)
	                 ? 4
	                 : 0;
	int index;
	for (index = 0; index < count; ++index)
	{
		offset += align_up(ir_type_size(hs.function->ir->parameterTypes[index]), 4);
	}
	return offset;
}

static void emit_ir_parameter(const IrInstruction *instruction)
{
	emit_op(NULL, "L", "6,%s(,13)", hs.function->params);
	add_immediate(6, c_parameter_offset(instruction->offset));
	if (instruction->type.kind == IR_TYPE_FLOAT)
	{
		emit_op(NULL, instruction->type.bits == 32U ? "LE" : "LD", "0,0(,6)");
		if (instruction->type.bits == 32U)
		{
			emit_op(NULL, "LDEBR", "0,0");
		}
		store_ir_float(0, instruction->result);
	}
	else
	{
		emit_op(NULL, "L", "2,0(,6)");
		if (instruction->type.kind == IR_TYPE_AGGREGATE && instruction->type.bits < 32U)
		{
			emit_op(NULL, "SRL", "2,%u", 32U - instruction->type.bits);
		}
		normalize_ir_integer(2, instruction->type);
		store_ir_integer(2, instruction->result);
	}
}

static void emit_ir_variadic(const IrInstruction *instruction)
{
	load_ir_integer(3, instruction->left);
	if (instruction->opcode == IR_OP_VA_START)
	{
		emit_op(NULL, "L", "2,%s(,13)", hs.function->params);
		add_immediate(2, c_parameter_offset(hs.function->ir->parameterCount));
		emit_op(NULL, "ST", "2,0(,3)");
	}
	else if (instruction->opcode == IR_OP_VA_COPY)
	{
		load_ir_integer(2, instruction->right);
		emit_op(NULL, "L", "2,0(,2)");
		emit_op(NULL, "ST", "2,0(,3)");
	}
	else if (instruction->opcode == IR_OP_VA_END)
	{
		emit_op(NULL, "XC", "0(4,3),0(3)");
	}
	else
	{
		emit_op(NULL, "L", "6,0(,3)");
		emit_op(NULL, "LR", "2,6");
		add_immediate(2, align_up(ir_type_size(instruction->type), 4));
		emit_op(NULL, "ST", "2,0(,3)");
		if (instruction->type.kind == IR_TYPE_FLOAT)
		{
			emit_op(NULL, "LD", "0,0(,6)");
			store_ir_float(0, instruction->result);
		}
		else
		{
			emit_op(NULL, "L", "2,0(,6)");
			if (instruction->type.kind == IR_TYPE_AGGREGATE && instruction->type.bits < 32U)
			{
				emit_op(NULL, "SRL", "2,%u", 32U - instruction->type.bits);
			}
			store_ir_integer(2, instruction->result);
		}
	}
}

static void emit_ir_call(const IrInstruction *instruction)
{
	char symbol[9];
	int argumentIndex;
	int offset = 0;
	if (hs.function->isMain && instruction->symbol >= 0 &&
	    strcmp(source_name(instruction->symbol), "main") == 0)
	{
		emit_main_arguments_and_call(instruction->symbol);
	}
	else
	{
		if (instruction->type.kind == IR_TYPE_FLOAT || instruction->type.kind == IR_TYPE_AGGREGATE)
		{
			address_from_base(2, hs.function->locals, ir_value_offset(instruction->result));
			address_from_base(1, hs.function->argList, 0);
			emit_op(NULL, "ST", "2,0(,1)");
			offset = 4;
		}
		for (argumentIndex = 0; argumentIndex < instruction->argumentCount; ++argumentIndex)
		{
			IrValueId argument = instruction->arguments[argumentIndex];
			IrType type = ir_value_type(argument);
			if (type.kind == IR_TYPE_FLOAT)
			{
				load_ir_float(0, argument);
				address_from_base(1, hs.function->argList, offset);
				if (type.bits == 32U)
				{
					emit_op(NULL, "LEDBR", "0,0");
				}
				emit_op(NULL, type.bits == 32U ? "STE" : "STD", "0,0(,1)");
			}
			else
			{
				load_ir_integer(2, argument);
				if (type.kind == IR_TYPE_AGGREGATE && type.bits < 32U)
				{
					emit_op(NULL, "SLL", "2,%u", 32U - type.bits);
				}
				address_from_base(1, hs.function->argList, offset);
				emit_op(NULL, "ST", "2,0(,1)");
			}
			offset += align_up(ir_type_size(type), 4);
		}
		if (instruction->opcode == IR_OP_CALL_INDIRECT)
		{
			load_ir_integer(15, instruction->left);
		}
		else
		{
			ir_symbol_name(symbol, instruction->symbol);
			emit_op(NULL, "LARL", "15,%s", symbol);
		}
		address_from_base(1, hs.function->argList, 0);
		emit_op(NULL, "BALR", "14,15");
		emit_op(NULL, "LR", "2,15");
	}
	if (instruction->result != IR_VALUE_NONE && instruction->type.kind != IR_TYPE_FLOAT &&
	    instruction->type.kind != IR_TYPE_AGGREGATE)
	{
		store_ir_integer(2, instruction->result);
	}
}

static void emit_ir_conversion(const IrInstruction *instruction)
{
	IrType source = ir_value_type(instruction->left);
	IrType target = instruction->type;
	if (source.kind == IR_TYPE_FLOAT && target.kind == IR_TYPE_FLOAT)
	{
		load_ir_float(0, instruction->left);
		store_ir_float(0, instruction->result);
	}
	else if (source.kind == IR_TYPE_FLOAT)
	{
		load_ir_float(0, instruction->left);
		emit_op(
		    NULL, target.isUnsigned ? "CLFDBR" : "CFDBR", target.isUnsigned ? "2,5,0,0" : "2,5,0");
		normalize_ir_integer(2, target);
		store_ir_integer(2, instruction->result);
	}
	else if (target.kind == IR_TYPE_FLOAT)
	{
		load_ir_integer(2, instruction->left);
		emit_op(
		    NULL, source.isUnsigned ? "CDLFBR" : "CDFBR", source.isUnsigned ? "0,0,2,0" : "0,2");
		store_ir_float(0, instruction->result);
	}
	else
	{
		load_ir_integer(2, instruction->left);
		normalize_ir_integer(2, target);
		store_ir_integer(2, instruction->result);
	}
}

static void emit_ir_binary(const IrInstruction *instruction)
{
	IrOpcode opcode = instruction->opcode;
	if (instruction->type.kind == IR_TYPE_FLOAT)
	{
		const char *operation = opcode == IR_OP_ADD             ? "ADBR"
		                        : opcode == IR_OP_SUBTRACT      ? "SDBR"
		                        : opcode == IR_OP_MULTIPLY      ? "MDBR"
		                        : opcode == IR_OP_DIVIDE_SIGNED ? "DDBR"
		                                                        : NULL;
		if (operation == NULL)
		{
			error("hlasm IR", "invalid binary64 opcode %d", opcode);
		}
		load_ir_float(0, instruction->left);
		load_ir_float(2, instruction->right);
		emit_op(NULL, operation, "0,2");
		store_ir_float(0, instruction->result);
		return;
	}
	load_ir_integer(2, instruction->left);
	load_ir_integer(3, instruction->right);
	switch (opcode)
	{
	case IR_OP_ADD:
		emit_op(NULL, "AR", "2,3");
		break;
	case IR_OP_SUBTRACT:
		emit_op(NULL, "SR", "2,3");
		break;
	case IR_OP_MULTIPLY:
		emit_op(NULL, "MSR", "2,3");
		break;
	case IR_OP_BITWISE_AND:
		emit_op(NULL, "NR", "2,3");
		break;
	case IR_OP_BITWISE_OR:
		emit_op(NULL, "OR", "2,3");
		break;
	case IR_OP_BITWISE_XOR:
		emit_op(NULL, "XR", "2,3");
		break;
	case IR_OP_SHIFT_LEFT:
		emit_op(NULL, "SLL", "2,0(3)");
		break;
	case IR_OP_SHIFT_RIGHT_SIGNED:
		emit_op(NULL, "SRA", "2,0(3)");
		break;
	case IR_OP_SHIFT_RIGHT_UNSIGNED:
		emit_op(NULL, "SRL", "2,0(3)");
		break;
	case IR_OP_DIVIDE_SIGNED:
	case IR_OP_REMAINDER_SIGNED:
		emit_op(NULL, "LR", "5,2");
		emit_op(NULL, "LR", "4,2");
		emit_op(NULL, "SRA", "4,31");
		emit_op(NULL, "DR", "4,3");
		emit_op(NULL, "LR", "2,%d", opcode == IR_OP_DIVIDE_SIGNED ? 5 : 4);
		break;
	case IR_OP_DIVIDE_UNSIGNED:
	case IR_OP_REMAINDER_UNSIGNED:
		emit_op(NULL, "LR", "5,2");
		emit_op(NULL, "XR", "4,4");
		emit_op(NULL, "DLR", "4,3");
		emit_op(NULL, "LR", "2,%d", opcode == IR_OP_DIVIDE_UNSIGNED ? 5 : 4);
		break;
	case IR_OP_POINTER_ADD:
	case IR_OP_POINTER_SUBTRACT:
		if (instruction->offset != 1)
		{
			load_immediate(0, instruction->offset);
			emit_op(NULL, "MSR", "3,0");
		}
		emit_op(NULL, opcode == IR_OP_POINTER_ADD ? "AR" : "SR", "2,3");
		break;
	case IR_OP_POINTER_DIFFERENCE:
		emit_op(NULL, "SR", "2,3");
		load_immediate(3, instruction->offset);
		emit_op(NULL, "LR", "5,2");
		emit_op(NULL, "LR", "4,2");
		emit_op(NULL, "SRA", "4,31");
		emit_op(NULL, "DR", "4,3");
		emit_op(NULL, "LR", "2,5");
		break;
	default:
		error("hlasm IR", "invalid integral binary opcode %d", opcode);
	}
	store_ir_integer(2, instruction->result);
}

static void emit_float_constant(const IrInstruction *instruction)
{
	unsigned char bytes[8];
	unsigned int endianProbe = 1U;
	int littleEndian = *(unsigned char *)&endianProbe != 0;
	char encoded[17];
	char constant[9];
	char continuation[9];
	int index;
	memcpy(bytes, &instruction->floating, sizeof(bytes));
	for (index = 0; index < 8; ++index)
	{
		(void)snprintf(encoded + index * 2,
		               sizeof(encoded) - (size_t)index * 2U,
		               "%02X",
		               bytes[littleEndian ? 7 - index : index]);
	}
	generated_name(constant);
	generated_name(continuation);
	emit_op(NULL, "J", "%s", continuation);
	emit_op(NULL, "DS", "0D");
	emit_op(constant, "DC", "X'%s'", encoded);
	emit_op(continuation, "LARL", "1,%s", constant);
	emit_op(NULL, "LD", "0,0(,1)");
	store_ir_float(0, instruction->result);
}

static void emit_ir_instruction(const IrInstruction *instruction)
{
	char label[9];
	switch (instruction->opcode)
	{
	case IR_OP_PARAMETER:
		emit_ir_parameter(instruction);
		break;
	case IR_OP_UNDEFINED:
		break;
	case IR_OP_CONSTANT_INTEGER:
		load_immediate(2, (int)(uint32_t)instruction->integer);
		store_ir_integer(2, instruction->result);
		break;
	case IR_OP_CONSTANT_FLOAT:
		emit_float_constant(instruction);
		break;
	case IR_OP_ADDRESS_OF:
		ir_symbol_name(label, instruction->symbol);
		emit_op(NULL, "LARL", "2,%s", label);
		if (instruction->offset != 0)
		{
			add_immediate(2, instruction->offset);
		}
		store_ir_integer(2, instruction->result);
		break;
	case IR_OP_LOCAL_ADDRESS:
		if (instruction->local < 0 || instruction->local >= hs.function->ir->localCount)
		{
			error("hlasm IR", "invalid local %d", instruction->local);
		}
		address_from_base(2,
		                  hs.function->locals,
		                  hs.function->localOffsets[instruction->local] + instruction->offset);
		store_ir_integer(2, instruction->result);
		break;
	case IR_OP_LOAD:
		load_ir_integer(2, instruction->left);
		if (instruction->type.kind == IR_TYPE_FLOAT)
		{
			emit_op(NULL, instruction->type.bits == 32U ? "LE" : "LD", "0,0(,2)");
			if (instruction->type.bits == 32U)
			{
				emit_op(NULL, "LDEBR", "0,0");
			}
			store_ir_float(0, instruction->result);
		}
		else
		{
			emit_op(NULL,
			        instruction->type.bits == 32U   ? "L"
			        : instruction->type.bits == 16U ? (instruction->type.isUnsigned ? "LLH" : "LH")
			                                        : "LLGC",
			        "2,0(,2)");
			if (instruction->type.bits == 8U && !instruction->type.isUnsigned)
			{
				normalize_ir_integer(2, instruction->type);
			}
			store_ir_integer(2, instruction->result);
		}
		break;
	case IR_OP_STORE:
		load_ir_integer(3, instruction->left);
		if (instruction->type.kind == IR_TYPE_FLOAT)
		{
			load_ir_float(0, instruction->right);
			if (instruction->type.bits == 32U)
			{
				emit_op(NULL, "LEDBR", "0,0");
			}
			emit_op(NULL, instruction->type.bits == 32U ? "STE" : "STD", "0,0(,3)");
		}
		else
		{
			load_ir_integer(2, instruction->right);
			emit_op(NULL,
			        instruction->type.bits == 32U   ? "ST"
			        : instruction->type.bits == 16U ? "STH"
			                                        : "STC",
			        "2,0(,3)");
		}
		break;
	case IR_OP_ZERO_MEMORY:
	{
		int remaining = instruction->offset;
		load_ir_integer(2, instruction->left);
		while (remaining > 0)
		{
			int count = remaining > 256 ? 256 : remaining;
			emit_op(NULL, "XC", "0(%d,2),0(2)", count);
			remaining -= count;
			if (remaining > 0)
			{
				add_immediate(2, count);
			}
		}
		break;
	}
	case IR_OP_COPY:
		if (instruction->type.kind == IR_TYPE_FLOAT)
		{
			load_ir_float(0, instruction->left);
			store_ir_float(0, instruction->result);
		}
		else
		{
			load_ir_integer(2, instruction->left);
			store_ir_integer(2, instruction->result);
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
		emit_ir_binary(instruction);
		break;
	case IR_OP_NEGATE:
		if (instruction->type.kind == IR_TYPE_FLOAT)
		{
			load_ir_float(0, instruction->left);
			emit_op(NULL, "LCDBR", "0,0");
			store_ir_float(0, instruction->result);
		}
		else
		{
			load_ir_integer(2, instruction->left);
			emit_op(NULL, "LCR", "2,2");
			store_ir_integer(2, instruction->result);
		}
		break;
	case IR_OP_BITWISE_NOT:
		load_ir_integer(2, instruction->left);
		load_immediate(3, -1);
		emit_op(NULL, "XR", "2,3");
		store_ir_integer(2, instruction->result);
		break;
	case IR_OP_COMPARE:
	{
		IrType operandType = ir_value_type(instruction->left);

		if (operandType.kind == IR_TYPE_FLOAT)
		{
			load_ir_float(0, instruction->left);
			load_ir_float(2, instruction->right);
			emit_op(NULL, "CDBR", "0,2");
			if (instruction->condition == IR_COMPARE_LESS_EQUAL_SIGNED ||
			    instruction->condition == IR_COMPARE_GREATER_EQUAL_SIGNED)
			{
				char yes[9];
				char done[9];
				generated_name(yes);
				generated_name(done);
				emit_op(NULL,
				        "BRCL",
				        "%d,%s",
				        instruction->condition == IR_COMPARE_LESS_EQUAL_SIGNED ? 12 : 10,
				        yes);
				emit_op(NULL, "XR", "2,2");
				emit_op(NULL, "J", "%s", done);
				emit_op(yes, "LHI", "2,1");
				emit_op(done, "DS", "0H");
				store_ir_integer(2, instruction->result);
				break;
			}
		}
		else
		{
			load_ir_integer(2, instruction->left);
			load_ir_integer(3, instruction->right);
			emit_op(NULL, ir_comparison_is_unsigned(instruction->condition) ? "CLR" : "CR", "2,3");
		}
		set_condition_result(ir_comparison_branch(instruction->condition));
		store_ir_integer(2, instruction->result);
		break;
	}
	case IR_OP_CONVERT:
		emit_ir_conversion(instruction);
		break;
	case IR_OP_CALL:
	case IR_OP_CALL_INDIRECT:
		emit_ir_call(instruction);
		break;
	case IR_OP_VA_START:
	case IR_OP_VA_ARGUMENT:
	case IR_OP_VA_COPY:
	case IR_OP_VA_END:
		emit_ir_variadic(instruction);
		break;
	case IR_OP_BRANCH:
		location_name(label, instruction->trueBlock);
		emit_op(NULL, "J", "%s", label);
		break;
	case IR_OP_BRANCH_CONDITIONAL:
		load_ir_integer(2, instruction->left);
		emit_op(NULL, "LTR", "2,2");
		location_name(label, instruction->trueBlock);
		emit_op(NULL, "JNE", "%s", label);
		location_name(label, instruction->falseBlock);
		emit_op(NULL, "J", "%s", label);
		break;
	case IR_OP_RETURN:
		if (instruction->type.kind == IR_TYPE_FLOAT)
		{
			load_ir_float(0, instruction->left);
			emit_op(NULL, "L", "6,%s(,13)", hs.function->params);
			emit_op(NULL, "L", "6,0(,6)");
			if (instruction->type.bits == 32U)
			{
				emit_op(NULL, "LEDBR", "0,0");
			}
			emit_op(NULL, instruction->type.bits == 32U ? "STE" : "STD", "0,0(,6)");
			emit_op(NULL, "CEETERM", "RC=0,MODIFIER=0");
		}
		else if (instruction->type.kind == IR_TYPE_AGGREGATE)
		{
			load_ir_integer(2, instruction->left);
			emit_op(NULL, "L", "6,%s(,13)", hs.function->params);
			emit_op(NULL, "L", "6,0(,6)");
			emit_op(NULL,
			        instruction->type.bits == 8U    ? "STC"
			        : instruction->type.bits == 16U ? "STH"
			                                        : "ST",
			        "2,0(,6)");
			emit_op(NULL, "CEETERM", "RC=0,MODIFIER=0");
		}
		else if (instruction->type.kind == IR_TYPE_VOID)
		{
			emit_op(NULL, "CEETERM", "RC=0,MODIFIER=0");
		}
		else
		{
			load_ir_integer(2, instruction->left);
			emit_op(NULL, "CEETERM", "RC=(2),MODIFIER=0");
		}
		break;
	default:
		error("hlasm IR", "opcode %d has no complete lowering", instruction->opcode);
	}
}

static void emit_ir_external_symbols(void)
{
	int *symbols = xalloc((size_t)cd.hash.size * sizeof(*symbols));
	int count = 0;
	int functionIndex;
	int globalIndex;
	for (functionIndex = 0; functionIndex < compiler.ir.functionCount; ++functionIndex)
	{
		const IrFunction *function = &compiler.ir.functions[functionIndex];
		int blockIndex;
		for (blockIndex = 0; blockIndex < function->blockCount; ++blockIndex)
		{
			const IrBasicBlock *block = &function->blocks[blockIndex];
			int instructionIndex;
			for (instructionIndex = 0; instructionIndex < block->instructionCount;
			     ++instructionIndex)
			{
				const IrInstruction *instruction = &block->instructions[instructionIndex];
				int symbol = instruction->symbol;
				char name[9];
				Name *globalName;
				int external = 0;
				if (symbol < 0 ||
				    (instruction->opcode != IR_OP_CALL && instruction->opcode != IR_OP_ADDRESS_OF))
				{
					continue;
				}
				globalName = getNameFromTable(globTable, NM_VAR, symbol);
				if (instruction->opcode == IR_OP_CALL)
				{
					external = !is_defined_function(symbol);
				}
				else if (globalName != NULL)
				{
					external = globalName->addrType == AD_IMPORT;
				}
				else
				{
					external = !is_defined_function(symbol);
				}
				if (!external || symbol_was_emitted(symbols, count, symbol))
				{
					continue;
				}
				symbols[count++] = symbol;
				symbol_name(name, symbol);
				emit_op(NULL, "EXTRN", "%s", name);
				emit_alias(name, source_name(symbol));
			}
		}
	}
	for (globalIndex = 0; globalIndex < compiler.ir.globalCount; ++globalIndex)
	{
		const IrGlobal *global = &compiler.ir.globals[globalIndex];
		int relocationIndex;

		for (relocationIndex = 0; relocationIndex < global->relocationCount; ++relocationIndex)
		{
			int symbol = global->relocations[relocationIndex].symbol;
			IrGlobal *target = irFindGlobal(&compiler.ir, symbol);
			char name[9];

			if (symbol < 0 || is_defined_function(symbol) ||
			    (target != NULL && !target->isExternal) ||
			    symbol_was_emitted(symbols, count, symbol))
			{
				continue;
			}
			symbols[count++] = symbol;
			symbol_name(name, symbol);
			emit_op(NULL, "EXTRN", "%s", name);
			emit_alias(name, source_name(symbol));
		}
	}
	free(symbols);
}

static void emit_ir_program(void)
{
	char entry[9] = "";
	int functionIndex;
	emit("* Generated by cc for z/OS LE non-XPLINK");
	emit_op("CCPROG", "RSECT", "");
	emit_op("CCPROG", "AMODE", "31");
	emit_op("CCPROG", "RMODE", "ANY");
	emit_ir_external_symbols();
	for (functionIndex = 0; functionIndex < hs.functionCount; ++functionIndex)
	{
		HLASM_FUNCTION *function = &hs.functions[functionIndex];
		if (function->ir->isInternal)
		{
			emit_unit_alias(function->entry, "function", function->ordinal);
			continue;
		}
		{
			const char *name = source_name(function->id);

			if (strcmp(name, "_main") == 0)
			{
				strcpy(entry, function->entry);
			}
			emit_op(NULL, "ENTRY", "%s", function->entry);
			emit_alias(function->entry, name);
		}
	}
	for (functionIndex = 0; functionIndex < hs.functionCount; ++functionIndex)
	{
		HLASM_FUNCTION *function = &hs.functions[functionIndex];
		int blockIndex;
		emit_function_entry(function);
		for (blockIndex = 0; blockIndex < function->ir->blockCount; ++blockIndex)
		{
			const IrBasicBlock *block = &function->ir->blocks[blockIndex];
			int instructionIndex;
			emit_ir_block_label(block->id);
			for (instructionIndex = 0; instructionIndex < block->instructionCount;
			     ++instructionIndex)
			{
				emit_ir_instruction(&block->instructions[instructionIndex]);
			}
		}
	}
	emit_data();
	emit_dsects();
	emit_op(NULL, "END", "%s", entry);
}

void hlasm_link(const char *outputFile)
{
	int functionIndex;
	const unsigned char *unit = (const unsigned char *)mcc.srcFile[mcc.mainfile];
	uint32_t first = 2166136261U;
	uint32_t second = 5381U;
	memset(&hs, 0, sizeof(hs));
	while (*unit != 0U)
	{
		unsigned int byte = *unit++;
		first = (first ^ byte) * 16777619U;
		second = ((second << 5) + second) ^ byte;
	}
	(void)snprintf(hs.unitName, sizeof(hs.unitName), "%08X%08X", first, second);
	analyze_ir_functions();
	hs.output = fopen(outputFile, "w");
	if (hs.output == NULL)
	{
		error("hlasm", "cannot open output file '%s'", outputFile);
	}
	emit_ir_program();
	if (fclose(hs.output) != 0)
	{
		error("hlasm", "cannot close output file '%s'", outputFile);
	}
	hs.output = NULL;
	for (functionIndex = 0; functionIndex < hs.functionCount; ++functionIndex)
	{
		free(hs.functions[functionIndex].localOffsets);
		free(hs.functions[functionIndex].valueOffsets);
		free(hs.functions[functionIndex].valueTypes);
	}
	free(hs.functions);
	hs.functions = NULL;
	printf("HLASM output written to: %s\n", outputFile);
}

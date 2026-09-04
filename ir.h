/* Architecture-neutral, typed control-flow graph IR. */

#ifndef CC_IR_H
#define CC_IR_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef int IrValueId;
typedef int IrBlockId;
typedef int IrSymbolId;
typedef int IrLocalId;

#define IR_VALUE_NONE (-1)
#define IR_BLOCK_NONE (-1)
#define IR_SYMBOL_NONE (-1)
#define IR_LOCAL_NONE (-1)

typedef enum
{
	IR_TYPE_VOID = 0,
	IR_TYPE_INTEGER,
	IR_TYPE_FLOAT,
	IR_TYPE_POINTER
} IrTypeKind;

typedef struct _IrType
{
	unsigned char kind;
	unsigned char bits;
	unsigned char isUnsigned;
	unsigned char alignment;
} IrType;

typedef enum
{
	IR_OP_PARAMETER = 0,
	IR_OP_UNDEFINED,
	IR_OP_CONSTANT_INTEGER,
	IR_OP_CONSTANT_FLOAT,
	IR_OP_ADDRESS_OF,
	IR_OP_LOCAL_ADDRESS,
	IR_OP_LOAD,
	IR_OP_STORE,
	IR_OP_ZERO_MEMORY,
	IR_OP_COPY,
	IR_OP_ADD,
	IR_OP_SUBTRACT,
	IR_OP_MULTIPLY,
	IR_OP_DIVIDE_SIGNED,
	IR_OP_DIVIDE_UNSIGNED,
	IR_OP_REMAINDER_SIGNED,
	IR_OP_REMAINDER_UNSIGNED,
	IR_OP_BITWISE_AND,
	IR_OP_BITWISE_OR,
	IR_OP_BITWISE_XOR,
	IR_OP_SHIFT_LEFT,
	IR_OP_SHIFT_RIGHT_SIGNED,
	IR_OP_SHIFT_RIGHT_UNSIGNED,
	IR_OP_POINTER_ADD,
	IR_OP_POINTER_SUBTRACT,
	IR_OP_POINTER_DIFFERENCE,
	IR_OP_NEGATE,
	IR_OP_BITWISE_NOT,
	IR_OP_COMPARE,
	IR_OP_CONVERT,
	IR_OP_CALL,
	IR_OP_CALL_INDIRECT,
	IR_OP_BRANCH,
	IR_OP_BRANCH_CONDITIONAL,
	IR_OP_RETURN
} IrOpcode;

typedef enum
{
	IR_COMPARE_EQUAL = 0,
	IR_COMPARE_NOT_EQUAL,
	IR_COMPARE_LESS_SIGNED,
	IR_COMPARE_LESS_UNSIGNED,
	IR_COMPARE_LESS_EQUAL_SIGNED,
	IR_COMPARE_LESS_EQUAL_UNSIGNED,
	IR_COMPARE_GREATER_SIGNED,
	IR_COMPARE_GREATER_UNSIGNED,
	IR_COMPARE_GREATER_EQUAL_SIGNED,
	IR_COMPARE_GREATER_EQUAL_UNSIGNED
} IrCompareCondition;

typedef enum
{
	IR_CALL_C = 0,
	IR_CALL_WINAPI
} IrCallingConvention;

typedef struct _IrInstruction
{
	IrOpcode opcode;
	IrType type;
	IrValueId result;
	IrValueId left;
	IrValueId right;
	uint32_t integer;
	double floating;
	int offset;
	IrSymbolId symbol;
	IrLocalId local;
	IrBlockId trueBlock;
	IrBlockId falseBlock;
	IrCompareCondition condition;
	IrCallingConvention callingConvention;
	IrValueId *arguments;
	int argumentCount;
	int sourceFile;
	int sourceLine;
} IrInstruction;

typedef struct _IrLocal
{
	IrLocalId id;
	IrSymbolId symbol;
	IrType type;
	size_t size;
	int alignment;
	int parameterIndex;
} IrLocal;

typedef struct _IrBasicBlock
{
	IrBlockId id;
	IrInstruction *instructions;
	int instructionCount;
	int instructionCapacity;
} IrBasicBlock;

typedef struct _IrFunction
{
	IrSymbolId symbol;
	IrType returnType;
	IrCallingConvention callingConvention;
	int isVariadic;
	int isInternal;
	int isExported;
	IrType *parameterTypes;
	int parameterCount;
	IrLocal *locals;
	int localCount;
	int localCapacity;
	IrBasicBlock *blocks;
	int blockCount;
	int blockCapacity;
	int nextValue;
} IrFunction;

typedef struct _IrGlobal
{
	IrSymbolId symbol;
	IrType type;
	unsigned char *initializer;
	size_t initializerSize;
	size_t zeroFillSize;
	int alignment;
	int isExternal;
	int isInternal;
	int isExported;
	struct _IrRelocation *relocations;
	int relocationCount;
	int relocationCapacity;
} IrGlobal;

typedef struct _IrRelocation
{
	size_t offset;
	IrSymbolId symbol;
	int addend;
} IrRelocation;

typedef struct _IrModule
{
	IrFunction *functions;
	int functionCount;
	int functionCapacity;
	IrGlobal *globals;
	int globalCount;
	int globalCapacity;
	IrSymbolId nextAnonymousSymbol;
} IrModule;

typedef struct _IrBuilder
{
	IrModule *module;
	IrFunction *function;
	IrBlockId block;
	int sourceFile;
	int sourceLine;
	int suspensionDepth;
} IrBuilder;

void irModuleInit(IrModule *module);
void irModuleFree(IrModule *module);
IrType irTypeVoid(void);
IrType irTypeInteger(unsigned int bits, int isUnsigned, unsigned int alignment);
IrType irTypeFloat(unsigned int bits, unsigned int alignment);
IrType irTypePointer(unsigned int bits, unsigned int alignment);
int irTypesEqual(IrType left, IrType right);
IrFunction *irAddFunction(IrModule *module, IrSymbolId symbol, IrType returnType);
IrSymbolId irCreateAnonymousSymbol(IrModule *module);
IrGlobal *irAddGlobal(IrModule *module,
                      IrSymbolId symbol,
                      IrType type,
                      size_t size,
                      int alignment,
                      int isExternal,
                      int isExported);
void irSetGlobalInitializer(IrGlobal *global, const void *bytes, size_t size);
IrGlobal *irFindGlobal(IrModule *module, IrSymbolId symbol);
void irAddGlobalRelocation(IrGlobal *global, size_t offset, IrSymbolId symbol, int addend);
void irSetFunctionParameters(IrFunction *function, const IrType *types, int count);
IrBasicBlock *irAddBlock(IrFunction *function);
IrLocalId irAddLocal(IrFunction *function,
                     IrSymbolId symbol,
                     IrType type,
                     size_t size,
                     int alignment,
                     int parameterIndex);
IrValueId irNextValue(IrFunction *function);
IrInstruction *irAppendInstruction(IrBasicBlock *block, IrOpcode opcode, IrType type);
void irSetCallArguments(IrInstruction *instruction, const IrValueId *arguments, int count);
void irBuilderInit(IrBuilder *builder, IrModule *module);
void irBuilderSetSource(IrBuilder *builder, int sourceFile, int sourceLine);
void irBuilderSuspend(IrBuilder *builder);
void irBuilderResume(IrBuilder *builder);
IrFunction *irBuilderBeginFunction(IrBuilder *builder,
                                   IrSymbolId symbol,
                                   IrType returnType,
                                   const IrType *parameterTypes,
                                   int parameterCount);
void irBuilderEndFunction(IrBuilder *builder);
IrBlockId irBuilderCreateBlock(IrBuilder *builder);
void irBuilderSetBlock(IrBuilder *builder, IrBlockId block);
int irBuilderBlockTerminated(const IrBuilder *builder);
IrValueId irBuilderEmitParameter(IrBuilder *builder, IrType type, int parameterIndex);
IrValueId irBuilderEmitUndefined(IrBuilder *builder, IrType type);
IrValueId irBuilderEmitInteger(IrBuilder *builder, IrType type, uint32_t value);
IrValueId irBuilderEmitFloat(IrBuilder *builder, IrType type, double value);
IrValueId
irBuilderEmitSymbolAddress(IrBuilder *builder, IrType type, IrSymbolId symbol, int offset);
IrValueId irBuilderEmitLocalAddress(IrBuilder *builder, IrType type, IrLocalId local, int offset);
IrValueId irBuilderEmitUnary(IrBuilder *builder, IrOpcode opcode, IrType type, IrValueId operand);
IrValueId irBuilderEmitBinary(
    IrBuilder *builder, IrOpcode opcode, IrType type, IrValueId left, IrValueId right);
IrValueId irBuilderEmitPointerOffset(IrBuilder *builder,
                                     IrOpcode opcode,
                                     IrType resultType,
                                     IrValueId pointer,
                                     IrValueId index,
                                     int elementSize);
IrValueId irBuilderEmitCompare(IrBuilder *builder,
                               IrCompareCondition condition,
                               IrValueId left,
                               IrValueId right);
IrValueId irBuilderEmitLoad(IrBuilder *builder, IrType type, IrValueId address);
void irBuilderEmitStore(IrBuilder *builder, IrType type, IrValueId address, IrValueId value);
void irBuilderEmitZeroMemory(IrBuilder *builder, IrValueId address, size_t size);
IrValueId irBuilderEmitCall(IrBuilder *builder,
                            IrType type,
                            IrSymbolId symbol,
                            IrCallingConvention callingConvention,
                            const IrValueId *arguments,
                            int argumentCount);
IrValueId irBuilderEmitCallIndirect(IrBuilder *builder,
                                    IrType type,
                                    IrValueId callee,
                                    IrCallingConvention callingConvention,
                                    const IrValueId *arguments,
                                    int argumentCount);
void irBuilderEmitBranch(IrBuilder *builder, IrBlockId destination);
void irBuilderEmitConditionalBranch(IrBuilder *builder,
                                    IrValueId condition,
                                    IrBlockId trueBlock,
                                    IrBlockId falseBlock);
void irBuilderEmitReturn(IrBuilder *builder, IrValueId value);
void irVerifyModule(const IrModule *module);
void irDumpModule(const IrModule *module, FILE *output);

#endif /* CC_IR_H */

/* Architecture-neutral expression emission for the C frontend. */

#include "cc.h"

static IrType intType(void)
{
	return irTypeForCObject(ID.T_INT, 0);
}

static IrType valueType(const VALUE *value)
{
	return irTypeForCObject(value->type, value->ptrs);
}

int semanticIrActive(void)
{
	irBuilderSetSource(
	    &compiler.irBuilder, cd.token[ix.tix].filenumber, cd.token[ix.tix].linenumber);
	return compiler.irBuilder.function != NULL && compiler.irBuilder.suspensionDepth == 0;
}

static IrValueId convertValue(IrValueId value, IrType source, IrType target)
{
	if (irTypesEqual(source, target))
	{
		return value;
	}
	return irBuilderEmitUnary(&compiler.irBuilder, IR_OP_CONVERT, target, value);
}

static IrValueId requireValue(VALUE *value)
{
	if (value->irValue != IR_VALUE_NONE)
	{
		return value->irValue;
	}
	if (value->irAddress != IR_VALUE_NONE)
	{
		semanticIrLoad(value);
		return value->irValue;
	}
	error("semantic IR", "expression has neither a value nor an address");
}

void semanticIrConstantInteger(VALUE *value)
{
	if (!semanticIrActive())
	{
		return;
	}
	value->irValue =
	    irBuilderEmitInteger(&compiler.irBuilder, valueType(value), (uint32_t)value->ival);
}

void semanticIrConstantFloat(VALUE *value)
{
	if (!semanticIrActive())
	{
		return;
	}
	value->irValue = irBuilderEmitFloat(&compiler.irBuilder, valueType(value), value->rval);
}

static int isNumericEscape(const char *source)
{
	return source[0] == '\\' &&
	       (source[1] == 'x' || source[1] == 'X' || (source[1] >= '0' && source[1] <= '7'));
}

unsigned char *semanticIrDecodeExecutionString(const char *literal, size_t *size)
{
	size_t capacity = (size_t)decodedStringLength((char *)literal) + 1U;
	unsigned char *bytes = xalloc(capacity);
	size_t length = 0U;
	const char *source = literal;
	while (*source != '\0')
	{
		int numericEscape = isNumericEscape(source);
		int value;
		if (*source == '\\')
		{
			++source;
			value = decodeEscapeSequence(&source);
		}
		else
		{
			value = (unsigned char)*source++;
		}
		if (value < 0 || value > UCHAR_MAX)
		{
			free(bytes);
			error("string literal", "escape value 0x%X does not fit in one byte", value);
		}
		bytes[length++] = numericEscape
		                      ? (unsigned char)value
		                      : cmd.target->encodeExecutionByte(&compiler, (unsigned char)value);
	}
	bytes[length++] = 0U;
	*size = length;
	return bytes;
}

IrSymbolId semanticIrCreateStringGlobal(const char *literal)
{
	IrSymbolId symbol;
	IrGlobal *global;
	unsigned char *bytes;
	size_t size;
	bytes = semanticIrDecodeExecutionString(literal, &size);
	symbol = irCreateAnonymousSymbol(&compiler.ir);
	global = irAddGlobal(&compiler.ir, symbol, irTypeInteger(CHAR_BIT, 1, 1U), size, 1, 0, 0);
	irSetGlobalInitializer(global, bytes, size);
	free(bytes);
	return symbol;
}

void semanticIrString(VALUE *value, const char *literal)
{
	IrSymbolId symbol;
	if (!semanticIrActive())
	{
		return;
	}
	symbol = semanticIrCreateStringGlobal(literal);
	value->irValue =
	    irBuilderEmitSymbolAddress(&compiler.irBuilder, irTypeForCObject(ID.T_CHAR, 1), symbol, 0);
	value->irAddress = value->irValue;
}

void semanticIrNameAddress(VALUE *value, const Name *name)
{
	IrType pointerType;
	if (!semanticIrActive())
	{
		return;
	}
	pointerType = irTypePointer((unsigned int)cmd.target->dataLayout.pointerSize * CHAR_BIT,
	                            cmd.target->dataLayout.pointerAlignment);
	if (name->addrType == AD_STACK)
	{
		if (name->irLocal == IR_LOCAL_NONE)
		{
			error("semantic IR", "local '%s' has no abstract storage", toString(name->idName));
		}
		value->irAddress =
		    irBuilderEmitLocalAddress(&compiler.irBuilder, pointerType, name->irLocal, 0);
	}
	else
	{
		if (name->irSymbol == IR_SYMBOL_NONE)
		{
			error("semantic IR", "symbol '%s' has no IR identity", toString(name->idName));
		}
		value->irAddress =
		    irBuilderEmitSymbolAddress(&compiler.irBuilder, pointerType, name->irSymbol, 0);
	}
}

void semanticIrLocalAddress(VALUE *value, IrLocalId local, int offset)
{
	IrType pointerType;
	if (!semanticIrActive())
	{
		return;
	}
	pointerType = irTypePointer((unsigned int)cmd.target->dataLayout.pointerSize * CHAR_BIT,
	                            cmd.target->dataLayout.pointerAlignment);
	value->irAddress = irBuilderEmitLocalAddress(&compiler.irBuilder, pointerType, local, offset);
}

void semanticIrLoad(VALUE *value)
{
	if (!semanticIrActive())
	{
		return;
	}
	if (value->irAddress == IR_VALUE_NONE)
	{
		error("semantic IR", "load requires an address");
	}
	value->irValue = irBuilderEmitLoad(&compiler.irBuilder, valueType(value), value->irAddress);
}

void semanticIrOffsetAddress(VALUE *base, VALUE *index, int scale)
{
	IrValueId address;
	IrValueId indexValue;
	if (!semanticIrActive())
	{
		return;
	}
	address = base->irAddress != IR_VALUE_NONE ? base->irAddress : requireValue(base);
	indexValue = requireValue(index);
	base->irAddress = irBuilderEmitPointerOffset(
	    &compiler.irBuilder,
	    IR_OP_POINTER_ADD,
	    irTypePointer((unsigned int)cmd.target->dataLayout.pointerSize * CHAR_BIT,
	                  cmd.target->dataLayout.pointerAlignment),
	    address,
	    indexValue,
	    scale);
	base->irValue = IR_VALUE_NONE;
}

void semanticIrAddAddressOffset(VALUE *value, int offset)
{
	VALUE constant;
	if (!semanticIrActive() || offset == 0)
	{
		return;
	}
	setValue(VAL, 0, ID.T_INT, &constant);
	constant.ival = offset;
	constant.fConst = TRUE;
	semanticIrConstantInteger(&constant);
	semanticIrOffsetAddress(value, &constant, 1);
}

static IrCompareCondition compareCondition(int operation, int unsignedComparison)
{
	if (operation == id2("=="))
	{
		return IR_COMPARE_EQUAL;
	}
	if (operation == id2("!="))
	{
		return IR_COMPARE_NOT_EQUAL;
	}
	if (operation == '<')
	{
		return unsignedComparison ? IR_COMPARE_LESS_UNSIGNED : IR_COMPARE_LESS_SIGNED;
	}
	if (operation == '>')
	{
		return unsignedComparison ? IR_COMPARE_GREATER_UNSIGNED : IR_COMPARE_GREATER_SIGNED;
	}
	if (operation == id2("<="))
	{
		return unsignedComparison ? IR_COMPARE_LESS_EQUAL_UNSIGNED : IR_COMPARE_LESS_EQUAL_SIGNED;
	}
	return unsignedComparison ? IR_COMPARE_GREATER_EQUAL_UNSIGNED : IR_COMPARE_GREATER_EQUAL_SIGNED;
}

static IrOpcode arithmeticOpcode(int operation, int unsignedOperation)
{
	if (operation == '+')
	{
		return IR_OP_ADD;
	}
	if (operation == '-')
	{
		return IR_OP_SUBTRACT;
	}
	if (operation == '*')
	{
		return IR_OP_MULTIPLY;
	}
	if (operation == '/')
	{
		return unsignedOperation ? IR_OP_DIVIDE_UNSIGNED : IR_OP_DIVIDE_SIGNED;
	}
	if (operation == '%')
	{
		return unsignedOperation ? IR_OP_REMAINDER_UNSIGNED : IR_OP_REMAINDER_SIGNED;
	}
	if (operation == '&')
	{
		return IR_OP_BITWISE_AND;
	}
	if (operation == '|')
	{
		return IR_OP_BITWISE_OR;
	}
	if (operation == '^')
	{
		return IR_OP_BITWISE_XOR;
	}
	if (operation == FRONTEND_OP_SHIFT_LEFT)
	{
		return IR_OP_SHIFT_LEFT;
	}
	if (operation == FRONTEND_OP_SHIFT_RIGHT)
	{
		return unsignedOperation ? IR_OP_SHIFT_RIGHT_UNSIGNED : IR_OP_SHIFT_RIGHT_SIGNED;
	}
	error("semantic IR", "operator '%s' has no arithmetic opcode", toString(operation));
}

static int baseAssignmentOperator(int operation)
{
	if (operation == id2("+="))
	{
		return '+';
	}
	if (operation == id2("-="))
	{
		return '-';
	}
	if (operation == id2("*="))
	{
		return '*';
	}
	if (operation == id2("/="))
	{
		return '/';
	}
	if (operation == id2("%="))
	{
		return '%';
	}
	if (operation == id2("&="))
	{
		return '&';
	}
	if (operation == id2("|="))
	{
		return '|';
	}
	if (operation == id2("^="))
	{
		return '^';
	}
	if (operation == id2("<<="))
	{
		return FRONTEND_OP_SHIFT_LEFT;
	}
	if (operation == id2(">>="))
	{
		return FRONTEND_OP_SHIFT_RIGHT;
	}
	return 0;
}

static IrValueId emitArithmetic(int operation, VALUE *left, VALUE *right, int resultType)
{
	IrValueId leftValue = requireValue(left);
	IrValueId rightValue = requireValue(right);
	IrType result = irTypeForCObject(resultType, 0);
	IrType leftType = valueType(left);
	IrType rightType = valueType(right);
	leftValue = convertValue(leftValue, leftType, result);
	rightValue = convertValue(rightValue, rightType, result);
	return irBuilderEmitBinary(&compiler.irBuilder,
	                           arithmeticOpcode(operation, isUnsignedType(resultType)),
	                           result,
	                           leftValue,
	                           rightValue);
}

void semanticIrBinary(int operation, VALUE *left, VALUE *right)
{
	int assignmentOperation;
	int comparison;
	int resultType;
	if (!semanticIrActive())
	{
		return;
	}
	assignmentOperation = baseAssignmentOperator(operation);
	if (operation == '=' || assignmentOperation != 0)
	{
		IrType targetType;
		IrValueId result;
		if (left->irAddress == IR_VALUE_NONE)
		{
			error("semantic IR", "assignment requires an address");
		}
		targetType = valueType(left);
		if (assignmentOperation == 0)
		{
			if (canonicalType(left->type) == ID.T_BOOL && left->ptrs == 0)
			{
				VALUE assigned;
				VALUE *source = left->irValue != IR_VALUE_NONE ? left : right;

				memcpy(&assigned, source, sizeof(assigned));
				result = semanticIrCondition(&assigned);
				result = convertValue(result, intType(), targetType);
			}
			else if (left->irValue != IR_VALUE_NONE)
			{
				result = left->irValue;
			}
			else
			{
				result = convertValue(requireValue(right), valueType(right), targetType);
			}
		}
		else
		{
			VALUE stored;

			memcpy(&stored, left, sizeof(stored));
			stored.irValue = irBuilderEmitLoad(&compiler.irBuilder, targetType, left->irAddress);
			stored.irAddress = IR_VALUE_NONE;
			if (stored.ptrs > 0 && (assignmentOperation == '+' || assignmentOperation == '-'))
			{
				result = irBuilderEmitPointerOffset(
				    &compiler.irBuilder,
				    assignmentOperation == '+' ? IR_OP_POINTER_ADD : IR_OP_POINTER_SUBTRACT,
				    targetType,
				    stored.irValue,
				    requireValue(right),
				    sizeOfObjectType(stored.type, stored.ptrs - 1));
			}
			else
			{
				int promotedType = assignmentOperation == FRONTEND_OP_SHIFT_LEFT ||
				                           assignmentOperation == FRONTEND_OP_SHIFT_RIGHT
				                       ? integerPromotion(stored.type)
				                       : arithmeticType(stored.type, right->type);
				result = emitArithmetic(assignmentOperation, &stored, right, promotedType);
				if (canonicalType(left->type) == ID.T_BOOL)
				{
					VALUE truth;
					setValue(VAL, 0, promotedType, &truth);
					truth.irValue = result;
					result = semanticIrCondition(&truth);
					result = convertValue(result, intType(), targetType);
				}
				else
				{
					result = convertValue(result, irTypeForCObject(promotedType, 0), targetType);
				}
			}
			if (canonicalType(left->type) == ID.T_BOOL && left->ptrs == 0)
			{
				VALUE normalized;

				memcpy(&normalized, left, sizeof(normalized));
				normalized.irValue = result;
				normalized.irAddress = IR_VALUE_NONE;
				result = semanticIrCondition(&normalized);
				result = convertValue(result, intType(), targetType);
			}
		}
		irBuilderEmitStore(&compiler.irBuilder, targetType, left->irAddress, result);
		left->irValue = result;
		return;
	}

	comparison = operation == id2("==") || operation == id2("!=") || operation == '<' ||
	             operation == '>' || operation == id2("<=") || operation == id2(">=");
	if (comparison)
	{
		IrValueId leftValue = requireValue(left);
		IrValueId rightValue = requireValue(right);
		IrType common;
		int unsignedComparison;
		if (left->ptrs > 0 || right->ptrs > 0)
		{
			common = irTypePointer((unsigned int)cmd.target->dataLayout.pointerSize * CHAR_BIT,
			                       cmd.target->dataLayout.pointerAlignment);
			unsignedComparison = TRUE;
		}
		else if (isFloatingType(left->type) || isFloatingType(right->type))
		{
			common = irTypeForCObject(arithmeticType(left->type, right->type), 0);
			unsignedComparison = FALSE;
		}
		else
		{
			int commonType = usualIntegerType(left->type, right->type);
			common = irTypeForCObject(commonType, 0);
			unsignedComparison = isUnsignedType(commonType);
		}
		leftValue = convertValue(leftValue, valueType(left), common);
		rightValue = convertValue(rightValue, valueType(right), common);
		left->irValue = irBuilderEmitCompare(&compiler.irBuilder,
		                                     compareCondition(operation, unsignedComparison),
		                                     leftValue,
		                                     rightValue);
		left->irAddress = IR_VALUE_NONE;
		return;
	}

	if (left->ptrs > 0 && right->ptrs == 0 && (operation == '+' || operation == '-'))
	{
		left->irValue = irBuilderEmitPointerOffset(&compiler.irBuilder,
		                                           operation == '+' ? IR_OP_POINTER_ADD
		                                                            : IR_OP_POINTER_SUBTRACT,
		                                           valueType(left),
		                                           requireValue(left),
		                                           requireValue(right),
		                                           sizeOfObjectType(left->type, left->ptrs - 1));
		left->irAddress = IR_VALUE_NONE;
		return;
	}
	if (left->ptrs > 0 && right->ptrs > 0 && operation == '-')
	{
		left->irValue = irBuilderEmitPointerOffset(&compiler.irBuilder,
		                                           IR_OP_POINTER_DIFFERENCE,
		                                           intType(),
		                                           requireValue(left),
		                                           requireValue(right),
		                                           sizeOfObjectType(left->type, left->ptrs - 1));
		left->irAddress = IR_VALUE_NONE;
		return;
	}

	if (left->ptrs == 0 && right->ptrs > 0 && operation == '+')
	{
		left->irValue = irBuilderEmitPointerOffset(&compiler.irBuilder,
		                                           IR_OP_POINTER_ADD,
		                                           valueType(right),
		                                           requireValue(right),
		                                           requireValue(left),
		                                           sizeOfObjectType(right->type, right->ptrs - 1));
		left->irAddress = IR_VALUE_NONE;
		return;
	}
	resultType = arithmeticType(left->type, right->type);
	if (operation == FRONTEND_OP_SHIFT_LEFT || operation == FRONTEND_OP_SHIFT_RIGHT)
	{
		resultType = integerPromotion(left->type);
	}
	left->irValue = emitArithmetic(operation, left, right, resultType);
	left->irAddress = IR_VALUE_NONE;
}

void semanticIrUnary(int operation, VALUE *value)
{
	IrValueId operand;
	IrType type;
	if (!semanticIrActive())
	{
		return;
	}
	operand = requireValue(value);
	type = valueType(value);
	if (operation != '!' && isIntegerType(value->type) && value->ptrs == 0)
	{
		IrType promoted = irTypeForCObject(integerPromotion(value->type), 0);
		operand = convertValue(operand, type, promoted);
		type = promoted;
	}
	if (operation == '+')
	{
		value->irValue = operand;
	}
	else if (operation == '-')
	{
		value->irValue = irBuilderEmitUnary(&compiler.irBuilder, IR_OP_NEGATE, type, operand);
	}
	else if (operation == '~')
	{
		value->irValue = irBuilderEmitUnary(&compiler.irBuilder, IR_OP_BITWISE_NOT, type, operand);
	}
	else if (operation == '!')
	{
		IrValueId zero = type.kind == IR_TYPE_FLOAT
		                     ? irBuilderEmitFloat(&compiler.irBuilder, type, 0.0)
		                     : irBuilderEmitInteger(&compiler.irBuilder, type, 0U);
		value->irValue = irBuilderEmitCompare(&compiler.irBuilder, IR_COMPARE_EQUAL, operand, zero);
	}
	else
	{
		error("semantic IR", "unsupported unary operator '%s'", toString(operation));
	}
	value->irAddress = IR_VALUE_NONE;
}

void semanticIrIncrement(VALUE *value, int increment, int postfix)
{
	IrType type;
	IrValueId oldValue;
	IrValueId amount;
	IrValueId newValue;
	int step;
	if (!semanticIrActive())
	{
		return;
	}
	if (value->irAddress == IR_VALUE_NONE)
	{
		error("semantic IR", "increment or decrement requires an address");
	}
	type = valueType(value);
	oldValue = irBuilderEmitLoad(&compiler.irBuilder, type, value->irAddress);
	step = value->ptrs > 0 ? sizeOfObjectType(value->type, value->ptrs - 1) : 1;
	amount = irBuilderEmitInteger(&compiler.irBuilder, intType(), 1U);
	if (value->ptrs > 0)
	{
		newValue =
		    irBuilderEmitPointerOffset(&compiler.irBuilder,
		                               increment ? IR_OP_POINTER_ADD : IR_OP_POINTER_SUBTRACT,
		                               type,
		                               oldValue,
		                               amount,
		                               step);
	}
	else
	{
		amount = convertValue(amount, intType(), type);
		newValue = irBuilderEmitBinary(
		    &compiler.irBuilder, increment ? IR_OP_ADD : IR_OP_SUBTRACT, type, oldValue, amount);
	}
	irBuilderEmitStore(&compiler.irBuilder, type, value->irAddress, newValue);
	value->irValue = postfix ? oldValue : newValue;
}

IrValueId semanticIrCondition(VALUE *value)
{
	IrValueId operand;
	IrValueId zero;
	IrType type;
	if (!semanticIrActive())
	{
		return IR_VALUE_NONE;
	}
	operand = requireValue(value);
	type = valueType(value);
	zero = type.kind == IR_TYPE_FLOAT ? irBuilderEmitFloat(&compiler.irBuilder, type, 0.0)
	                                  : irBuilderEmitInteger(&compiler.irBuilder, type, 0U);
	return irBuilderEmitCompare(&compiler.irBuilder, IR_COMPARE_NOT_EQUAL, operand, zero);
}

IrValueId semanticIrArgument(VALUE *value, int targetType, int targetPointers)
{
	IrValueId argument;
	IrType target;
	if (!semanticIrActive())
	{
		return IR_VALUE_NONE;
	}
	argument = requireValue(value);
	if (targetType == ID.DOTS3)
	{
		target =
		    value->ptrs > 0
		        ? valueType(value)
		        : irTypeForCObject(
		              isFloatingType(value->type) ? ID.T_DOUBLE : integerPromotion(value->type), 0);
		return convertValue(argument, valueType(value), target);
	}
	target = irTypeForCObject(targetType, targetPointers);
	return convertValue(argument, valueType(value), target);
}

void semanticIrCast(VALUE *value, int targetType, int targetPointers)
{
	IrType target;
	if (!semanticIrActive())
	{
		return;
	}
	if (canonicalType(targetType) == ID.T_VOID && targetPointers == 0)
	{
		if (value->type != ID.T_VOID || value->ptrs > 0)
		{
			(void)requireValue(value);
		}
		value->irValue = IR_VALUE_NONE;
		value->irAddress = IR_VALUE_NONE;
		return;
	}
	if (canonicalType(targetType) == ID.T_BOOL && targetPointers == 0)
	{
		value->irValue = semanticIrCondition(value);
		value->irValue = convertValue(value->irValue, intType(), irTypeForCObject(ID.T_BOOL, 0));
		value->irAddress = IR_VALUE_NONE;
		return;
	}
	target = irTypeForCObject(targetType, targetPointers);
	value->irValue = convertValue(requireValue(value), valueType(value), target);
	value->irAddress = IR_VALUE_NONE;
}

void semanticIrCall(VALUE *value,
                    const Name *function,
                    const IrValueId *arguments,
                    int argumentCount)
{
	IrCallingConvention convention;
	IrType resultType;
	if (!semanticIrActive())
	{
		return;
	}
	convention = (function->type & NM_WINAPI) != 0 ? IR_CALL_WINAPI : IR_CALL_C;
	resultType = irTypeForCObject(function->dataType, function->ptrs);
	value->irValue = irBuilderEmitCall(
	    &compiler.irBuilder, resultType, function->irSymbol, convention, arguments, argumentCount);
	value->irAddress = IR_VALUE_NONE;
}

void semanticIrCallIndirect(VALUE *value,
                            IrValueId callee,
                            const Name *signature,
                            const IrValueId *arguments,
                            int argumentCount)
{
	IrCallingConvention convention;
	IrType resultType;
	if (!semanticIrActive())
	{
		return;
	}
	if (signature == NULL || !signature->isFunctionPointer)
	{
		error("semantic IR", "indirect call requires a function-pointer signature");
	}
	convention = signature->functionCallConvention == NM_WINAPI ? IR_CALL_WINAPI : IR_CALL_C;
	resultType = irTypeForCObject(signature->dataType, signature->returnPointers);
	value->irValue = irBuilderEmitCallIndirect(
	    &compiler.irBuilder, resultType, callee, convention, arguments, argumentCount);
	value->irAddress = IR_VALUE_NONE;
}

void semanticIrReturn(VALUE *value)
{
	IrType returnType;
	IrValueId result = IR_VALUE_NONE;
	if (!semanticIrActive())
	{
		return;
	}
	returnType = compiler.irBuilder.function->returnType;
	if (returnType.kind == IR_TYPE_VOID)
	{
		if (value != NULL)
		{
			error("return", "void function cannot return a value");
		}
	}
	else
	{
		if (value == NULL)
		{
			error("return", "non-void function must return a value");
		}
		result = convertValue(requireValue(value), valueType(value), returnType);
	}
	irBuilderEmitReturn(&compiler.irBuilder, result);
}

IrBlockId semanticIrCreateBlock(void)
{
	if (!semanticIrActive())
	{
		return IR_BLOCK_NONE;
	}
	return irBuilderCreateBlock(&compiler.irBuilder);
}

void semanticIrSelectBlock(IrBlockId block)
{
	if (semanticIrActive())
	{
		irBuilderSetBlock(&compiler.irBuilder, block);
	}
}

void semanticIrBranch(IrBlockId destination)
{
	if (semanticIrActive() && !irBuilderBlockTerminated(&compiler.irBuilder))
	{
		irBuilderEmitBranch(&compiler.irBuilder, destination);
	}
}

void semanticIrBranchCondition(VALUE *condition, IrBlockId trueBlock, IrBlockId falseBlock)
{
	if (semanticIrActive())
	{
		irBuilderEmitConditionalBranch(
		    &compiler.irBuilder, semanticIrCondition(condition), trueBlock, falseBlock);
	}
}

IrLocalId semanticIrCreateTemporary(int type, int pointers)
{
	IrType irType;
	int size;
	if (!semanticIrActive())
	{
		return IR_LOCAL_NONE;
	}
	irType = irTypeForCObject(type, pointers);
	size = sizeOfObjectType(type, pointers);
	return irAddLocal(compiler.irBuilder.function,
	                  IR_SYMBOL_NONE,
	                  irType,
	                  (size_t)size,
	                  alignmentOfObjectType(type, pointers),
	                  -1);
}

void semanticIrStoreTemporary(IrLocalId local, int type, int pointers, VALUE *value)
{
	IrType target;
	IrType pointerType;
	IrValueId address;
	IrValueId stored;
	if (!semanticIrActive())
	{
		return;
	}
	target = irTypeForCObject(type, pointers);
	pointerType = irTypePointer((unsigned int)cmd.target->dataLayout.pointerSize * CHAR_BIT,
	                            cmd.target->dataLayout.pointerAlignment);
	address = irBuilderEmitLocalAddress(&compiler.irBuilder, pointerType, local, 0);
	stored = convertValue(requireValue(value), valueType(value), target);
	irBuilderEmitStore(&compiler.irBuilder, target, address, stored);
}

void semanticIrLoadTemporary(IrLocalId local, int type, int pointers, VALUE *value)
{
	IrType target;
	IrType pointerType;
	IrValueId address;
	if (!semanticIrActive())
	{
		return;
	}
	target = irTypeForCObject(type, pointers);
	pointerType = irTypePointer((unsigned int)cmd.target->dataLayout.pointerSize * CHAR_BIT,
	                            cmd.target->dataLayout.pointerAlignment);
	address = irBuilderEmitLocalAddress(&compiler.irBuilder, pointerType, local, 0);
	value->irAddress = IR_VALUE_NONE;
	value->irValue = irBuilderEmitLoad(&compiler.irBuilder, target, address);
}

void semanticIrZeroMemory(IrLocalId local, int offset, int size)
{
	IrType pointerType;
	IrValueId address;
	if (!semanticIrActive())
	{
		return;
	}
	pointerType = irTypePointer((unsigned int)cmd.target->dataLayout.pointerSize * CHAR_BIT,
	                            cmd.target->dataLayout.pointerAlignment);
	address = irBuilderEmitLocalAddress(&compiler.irBuilder, pointerType, local, offset);
	irBuilderEmitZeroMemory(&compiler.irBuilder, address, (size_t)size);
}

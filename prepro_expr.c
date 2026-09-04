#include "cc.h"

typedef struct _PP_EXPRESSION
{
	const char *cursor;
	HASH *macros;
	int depth;
	int evaluate;
} PP_EXPRESSION;

static void skipSpace(PP_EXPRESSION *expression)
{
	while (*expression->cursor == ' ' || *expression->cursor == '\t')
	{
		++expression->cursor;
	}
}

static int match(PP_EXPRESSION *expression, const char *token)
{
	size_t length;
	skipSpace(expression);
	length = strlen(token);
	if (strncmp(expression->cursor, token, length) != 0)
	{
		return 0;
	}
	expression->cursor += length;
	return 1;
}

static intmax_t conditional(PP_EXPRESSION *expression);

static intmax_t parseInteger(const char *text, char **end)
{
	const char *cursor = text;
	unsigned int base = 10;
	uintmax_t value = 0;
	int digits = 0;
	if (cursor[0] == '0' && (cursor[1] == 'x' || cursor[1] == 'X'))
	{
		base = 16;
		cursor += 2;
	}
	else if (cursor[0] == '0')
	{
		base = 8;
	}
	while (*cursor != '\0')
	{
		unsigned int digit;
		if (*cursor >= '0' && *cursor <= '9')
		{
			digit = (unsigned int)(*cursor - '0');
		}
		else if (*cursor >= 'a' && *cursor <= 'f')
		{
			digit = (unsigned int)(*cursor - 'a') + 10U;
		}
		else if (*cursor >= 'A' && *cursor <= 'F')
		{
			digit = (unsigned int)(*cursor - 'A') + 10U;
		}
		else
		{
			break;
		}
		if (digit >= base)
		{
			break;
		}
		if (value > (UINTMAX_MAX - digit) / base)
		{
			error("prepro", "integer constant is too large in #if expression");
		}
		value = value * base + digit;
		++cursor;
		++digits;
	}
	*end = (char *)(digits == 0 ? text : cursor);
	return (intmax_t)value;
}

static intmax_t evaluateNested(const char *text, HASH *macros, int depth, int evaluate)
{
	PP_EXPRESSION expression;
	if (depth > 128)
	{
		error("prepro", "recursive macro in #if expression");
	}
	expression.cursor = text;
	expression.macros = macros;
	expression.depth = depth;
	expression.evaluate = evaluate;
	return conditional(&expression);
}

static intmax_t primary(PP_EXPRESSION *expression)
{
	char name[256];
	size_t length = 0;
	char *end;
	intmax_t value;
	skipSpace(expression);
	if (match(expression, "("))
	{
		value = conditional(expression);
		if (!match(expression, ")"))
		{
			error("prepro", "')' expected in #if expression");
		}
		return value;
	}
	if (strncmp(expression->cursor, "defined", 7) == 0 &&
	    !isIdentifierContinue((unsigned char)expression->cursor[7]))
	{
		expression->cursor += 7;
		skipSpace(expression);
		if (match(expression, "("))
		{
			skipSpace(expression);
			while (isIdentifierContinue((unsigned char)*expression->cursor) &&
			       length + 1U < sizeof(name))
			{
				name[length++] = *expression->cursor++;
			}
			if (!match(expression, ")"))
			{
				error("prepro", "')' expected after defined");
			}
		}
		else
		{
			while (isIdentifierContinue((unsigned char)*expression->cursor) &&
			       length + 1U < sizeof(name))
			{
				name[length++] = *expression->cursor++;
			}
		}
		name[length] = '\0';
		if (length == 0U)
		{
			error("prepro", "identifier expected after defined");
		}
		return hashGet(name, expression->macros) != NULL;
	}
	if (isIdentifierStart((unsigned char)*expression->cursor))
	{
		const char *replacement;
		char *functionReplacement = NULL;
		while (isIdentifierContinue((unsigned char)*expression->cursor) &&
		       length + 1U < sizeof(name))
		{
			name[length++] = *expression->cursor++;
		}
		name[length] = '\0';
		if (preproExpandFunctionExpression(name, &expression->cursor, &functionReplacement))
		{
			value = evaluateNested(functionReplacement,
			                       expression->macros,
			                       expression->depth + 1,
			                       expression->evaluate);
			free(functionReplacement);
			return value;
		}
		replacement = hashGet(name, expression->macros);
		if (replacement == NULL || *replacement == '\0')
		{
			return 0;
		}
		return evaluateNested(
		    replacement, expression->macros, expression->depth + 1, expression->evaluate);
	}
	if (*expression->cursor == '\'')
	{
		++expression->cursor;
		if (*expression->cursor == '\\')
		{
			++expression->cursor;
			value = decodeEscapeSequence(&expression->cursor);
		}
		else
		{
			value = (unsigned char)*expression->cursor++;
		}
		if (*expression->cursor != '\'')
		{
			error("prepro", "invalid character constant in #if");
		}
		++expression->cursor;
		return value;
	}
	if (!isdigit((unsigned char)*expression->cursor))
	{
		error("prepro", "invalid token in #if expression near '%s'", expression->cursor);
	}
	value = parseInteger(expression->cursor, &end);
	if (end == expression->cursor)
	{
		error("prepro", "integer expected in #if expression");
	}
	while (*end == 'u' || *end == 'U' || *end == 'l' || *end == 'L')
	{
		++end;
	}
	expression->cursor = end;
	return value;
}

static intmax_t unary(PP_EXPRESSION *expression)
{
	if (match(expression, "+"))
	{
		return unary(expression);
	}
	if (match(expression, "-"))
	{
		return -unary(expression);
	}
	if (match(expression, "!"))
	{
		return !unary(expression);
	}
	if (match(expression, "~"))
	{
		return ~unary(expression);
	}
	return primary(expression);
}

static intmax_t multiplicative(PP_EXPRESSION *expression)
{
	intmax_t value = unary(expression);
	for (;;)
	{
		if (match(expression, "*"))
		{
			value *= unary(expression);
		}
		else if (match(expression, "/"))
		{
			intmax_t divisor = unary(expression);
			if (divisor == 0)
			{
				if (expression->evaluate)
				{
					error("prepro", "division by zero in #if expression");
				}
				value = 0;
			}
			else
			{
				value /= divisor;
			}
		}
		else if (match(expression, "%"))
		{
			intmax_t divisor = unary(expression);
			if (divisor == 0)
			{
				if (expression->evaluate)
				{
					error("prepro", "division by zero in #if expression");
				}
				value = 0;
			}
			else
			{
				value %= divisor;
			}
		}
		else
		{
			return value;
		}
	}
}

static intmax_t additive(PP_EXPRESSION *expression)
{
	intmax_t value = multiplicative(expression);
	for (;;)
	{
		if (match(expression, "+"))
		{
			value += multiplicative(expression);
		}
		else if (match(expression, "-"))
		{
			value -= multiplicative(expression);
		}
		else
		{
			return value;
		}
	}
}

static intmax_t shift(PP_EXPRESSION *expression)
{
	intmax_t value = additive(expression);
	for (;;)
	{
		intmax_t amount;
		if (match(expression, "<<"))
		{
			amount = additive(expression);
			if (amount < 0 || amount >= (intmax_t)(sizeof(intmax_t) * CHAR_BIT))
			{
				if (expression->evaluate)
				{
					error("prepro", "invalid shift in #if expression");
				}
				amount = 0;
			}
			value = (intmax_t)((uintmax_t)value << (unsigned int)amount);
		}
		else if (match(expression, ">>"))
		{
			amount = additive(expression);
			if (amount < 0 || amount >= (intmax_t)(sizeof(intmax_t) * CHAR_BIT))
			{
				if (expression->evaluate)
				{
					error("prepro", "invalid shift in #if expression");
				}
				amount = 0;
			}
			value >>= (unsigned int)amount;
		}
		else
		{
			return value;
		}
	}
}

static intmax_t relational(PP_EXPRESSION *expression)
{
	intmax_t value = shift(expression);
	for (;;)
	{
		if (match(expression, "<="))
		{
			value = value <= shift(expression);
		}
		else if (match(expression, ">="))
		{
			value = value >= shift(expression);
		}
		else if (match(expression, "<"))
		{
			value = value < shift(expression);
		}
		else if (match(expression, ">"))
		{
			value = value > shift(expression);
		}
		else
		{
			return value;
		}
	}
}

static intmax_t equality(PP_EXPRESSION *expression)
{
	intmax_t value = relational(expression);
	for (;;)
	{
		if (match(expression, "=="))
		{
			value = value == relational(expression);
		}
		else if (match(expression, "!="))
		{
			value = value != relational(expression);
		}
		else
		{
			return value;
		}
	}
}

static intmax_t bitAnd(PP_EXPRESSION *expression)
{
	intmax_t value = equality(expression);
	for (;;)
	{
		skipSpace(expression);
		if (strncmp(expression->cursor, "&&", 2) == 0 || *expression->cursor != '&')
		{
			return value;
		}
		++expression->cursor;
		value &= equality(expression);
	}
}

static intmax_t bitXor(PP_EXPRESSION *expression)
{
	intmax_t value = bitAnd(expression);
	while (match(expression, "^"))
	{
		value ^= bitAnd(expression);
	}
	return value;
}

static intmax_t bitOr(PP_EXPRESSION *expression)
{
	intmax_t value = bitXor(expression);
	for (;;)
	{
		skipSpace(expression);
		if (strncmp(expression->cursor, "||", 2) == 0 || *expression->cursor != '|')
		{
			return value;
		}
		++expression->cursor;
		value |= bitXor(expression);
	}
}

static intmax_t logicalAnd(PP_EXPRESSION *expression)
{
	intmax_t value = bitOr(expression);
	while (match(expression, "&&"))
	{
		int previous = expression->evaluate;
		intmax_t right;
		if (previous && !value)
		{
			expression->evaluate = 0;
		}
		right = bitOr(expression);
		expression->evaluate = previous;
		value = value && right;
	}
	return value;
}

static intmax_t logicalOr(PP_EXPRESSION *expression)
{
	intmax_t value = logicalAnd(expression);
	while (match(expression, "||"))
	{
		int previous = expression->evaluate;
		intmax_t right;
		if (previous && value)
		{
			expression->evaluate = 0;
		}
		right = logicalAnd(expression);
		expression->evaluate = previous;
		value = value || right;
	}
	return value;
}

static intmax_t conditional(PP_EXPRESSION *expression)
{
	intmax_t conditionValue = logicalOr(expression);
	if (match(expression, "?"))
	{
		int previous = expression->evaluate;
		intmax_t trueValue;
		intmax_t falseValue;
		if (previous && !conditionValue)
		{
			expression->evaluate = 0;
		}
		trueValue = conditional(expression);
		if (!match(expression, ":"))
		{
			error("prepro", "':' expected in #if expression");
		}
		expression->evaluate = previous && !conditionValue ? previous : 0;
		falseValue = conditional(expression);
		expression->evaluate = previous;
		return conditionValue ? trueValue : falseValue;
	}
	return conditionValue;
}

int preproEvalExpression(const char *text, HASH *macros)
{
	PP_EXPRESSION expression;
	intmax_t value;
	expression.cursor = text;
	expression.macros = macros;
	expression.depth = 0;
	expression.evaluate = 1;
	value = conditional(&expression);
	skipSpace(&expression);
	if (*expression.cursor != '\0')
	{
		error("prepro", "unexpected token in #if expression near '%s'", expression.cursor);
	}
	return value != 0;
}

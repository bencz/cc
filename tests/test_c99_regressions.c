#include <stdarg.h>

typedef struct _Buffer
{
	char bytes[7];
} Buffer;

enum
{
	ENUM_VALUE = 3
};

int declared_without_parameter_name(int);

int declared_without_parameter_name(int value)
{
	return value;
}

static int sum(int count, ...)
{
	va_list arguments;
	int total = 0;
	va_start(arguments, count);
	for (int index = 0; index < count; ++index)
	{
		total += va_arg(arguments, int);
	}
	va_end(arguments);
	return total;
}

int main(void)
{
	Buffer buffer;
	int value = 0;
	int *pointer = &value;
	unsigned int shifted = 0x80000000U;

	++*pointer;
	shifted >>= 28;
	shifted <<= 1;

	if (value != 1)
	{
		return 1;
	}
	if (!!value != 1 || ~0 != -1)
	{
		return 2;
	}
	if ((int)(unsigned char)257 != 1)
	{
		return 3;
	}
	if (shifted != 16U)
	{
		return 4;
	}
	if (sizeof(buffer.bytes) != 7)
	{
		return 5;
	}
	if (sizeof((char)1 + (char)2) != sizeof(int))
	{
		return 6;
	}
	if (declared_without_parameter_name(9) != 9)
	{
		return 7;
	}
	if (sum(4, 1, 2, 3, 4) != 10)
	{
		return 8;
	}
	if (sizeof(ENUM_VALUE) != sizeof(int))
	{
		return 9;
	}
	return 0;
}

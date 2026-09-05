#include <stdarg.h>

typedef struct Pair
{
	unsigned short first;
	unsigned short second;
} Pair;

typedef struct Byte
{
	unsigned char value;
} Byte;

static Pair swap(Pair pair)
{
	unsigned short first = pair.first;
	pair.first = pair.second;
	pair.second = first;
	return pair;
}

static Byte increment(Byte byte)
{
	byte.value += 1;
	return byte;
}

static int variadic_bytes(int count, ...)
{
	va_list arguments;
	int index;
	int sum = 0;
	va_start(arguments, count);
	for (index = 0; index < count; ++index)
	{
		Byte byte = va_arg(arguments, Byte);
		sum += byte.value;
	}
	va_end(arguments);
	return sum;
}

int main(void)
{
	Pair before = {17, 29};
	Pair after = swap(before);
	Byte small = {41};
	Byte next = increment(small);
	if (after.first != 29 || after.second != 17 || before.first != 17)
	{
		return 1;
	}
	return next.value != 42 || small.value != 41 ||
	       variadic_bytes(
	           10, small, small, small, small, small, small, small, small, small, small) != 410;
}

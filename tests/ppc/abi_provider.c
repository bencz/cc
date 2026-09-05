#include <stdarg.h>

typedef struct Pair
{
	unsigned short first;
	unsigned short second;
} Pair;

Pair cc_swap(Pair pair)
{
	unsigned short first = pair.first;
	pair.first = pair.second;
	pair.second = first;
	return pair;
}

int cc_callback(int (*callback)(int, double), int value)
{
	return callback(value, 2.5);
}

double cc_variadic(int count, ...)
{
	va_list arguments;
	double result = 0.0;
	int index;
	va_start(arguments, count);
	for (index = 0; index < count; ++index)
	{
		result += va_arg(arguments, int);
		result += va_arg(arguments, double);
	}
	va_end(arguments);
	return result;
}

extern double clang_variadic(int count, ...);
int cc_character(char value)
{
	return value;
}

double cc_call_clang(void)
{
	return clang_variadic(
	    10, 1, 1.0, 2, 2.0, 3, 3.0, 4, 4.0, 5, 5.0, 6, 6.0, 7, 7.0, 8, 8.0, 9, 9.0, 10, 10.0);
}

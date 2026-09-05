#include <stdarg.h>

typedef struct Pair
{
	unsigned short first;
	unsigned short second;
} Pair;

extern Pair cc_swap(Pair);
extern int cc_callback(int (*callback)(int, double), int);
extern double cc_variadic(int, ...);
extern double cc_call_clang(void);
extern int cc_character(char);

static int callback(int value, double increment)
{
	return value + (int)increment;
}

double clang_variadic(int count, ...)
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

int main(void)
{
	Pair original = {13, 29};
	Pair swapped = cc_swap(original);
	if (cc_character((char)255) != 255)
	{
		return 4;
	}
	if (swapped.first != 29 || swapped.second != 13 || original.first != 13)
	{
		return 1;
	}
	if (cc_callback(callback, 40) != 42)
	{
		return 2;
	}
	if (cc_variadic(
	        10, 1, 1.0, 2, 2.0, 3, 3.0, 4, 4.0, 5, 5.0, 6, 6.0, 7, 7.0, 8, 8.0, 9, 9.0, 10, 10.0) !=
	    110.0)
	{
		return 3;
	}
	return cc_call_clang() != 110.0;
}

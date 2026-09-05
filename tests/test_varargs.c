#include <stdarg.h>

static int sum(int count, ...)
{
	va_list arguments;
	va_list copy;
	int total = 0;
	int index;
	va_start(arguments, count);
	va_copy(copy, arguments);
	for (index = 0; index < count; ++index)
	{
		total += va_arg(arguments, int);
	}
	if (va_arg(copy, int) != 1)
	{
		return -1;
	}
	va_end(copy);
	va_end(arguments);
	return total;
}

static double mixed(int first, double fixed, ...)
{
	va_list arguments;
	double total = fixed;
	int index;
	va_start(arguments, fixed);
	for (index = 0; index < 10; ++index)
	{
		total += va_arg(arguments, double);
		total += va_arg(arguments, int);
	}
	va_end(arguments);
	return total + first;
}

int main(void)
{
	if (sum(10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10) != 55)
	{
		return 1;
	}
	if (mixed(1,
	          2.0,
	          1.0,
	          1,
	          2.0,
	          2,
	          3.0,
	          3,
	          4.0,
	          4,
	          5.0,
	          5,
	          6.0,
	          6,
	          7.0,
	          7,
	          8.0,
	          8,
	          9.0,
	          9,
	          10.0,
	          10) != 113.0)
	{
		return 2;
	}
	return 0;
}

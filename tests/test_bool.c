#include <stdbool.h>

int main(void)
{
	bool first = 7;
	_Bool second = 0;
	_Bool from_fraction = (_Bool)0.5;
	_Bool from_zero = (_Bool)0.0;

	second += 3;
	if (sizeof(_Bool) != 1)
	{
		return 1;
	}
	if (first != true || second != true)
	{
		return 2;
	}
	if (from_fraction != true || from_zero != false)
	{
		return 3;
	}
	first = 0;
	if (first != false)
	{
		return 4;
	}
	return 0;
}

enum Status
{
	STATUS_ZERO,
	STATUS_FOUR = 2 + 2,
	STATUS_FIVE,
	STATUS_NEGATIVE = -3
};

typedef enum Mode
{
	MODE_FIRST = STATUS_FOUR,
	MODE_SECOND
} Mode;

typedef enum
{
	ANONYMOUS_VALUE = 12
} Anonymous;

enum Status global_status = STATUS_FIVE;

int main(void)
{
	Mode mode = MODE_SECOND;
	Anonymous anonymous = ANONYMOUS_VALUE;

	if (STATUS_ZERO != 0)
	{
		return 1;
	}
	if (STATUS_FIVE != 5)
	{
		return 2;
	}
	if (STATUS_NEGATIVE != -3)
	{
		return 3;
	}
	if (global_status != STATUS_FIVE)
	{
		return 4;
	}
	if (mode != 5)
	{
		return 5;
	}
	if (anonymous != 12)
	{
		return 6;
	}
	if (sizeof(enum Status) != 4)
	{
		return 7;
	}
	return 0;
}

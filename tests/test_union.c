typedef union _Value
{
	unsigned int whole;
	unsigned short half;
	unsigned char byte;
	double real;
} Value;

Value initialized = {27U};

int main(void)
{
	Value value = {42U};

	if (sizeof(Value) != 8)
	{
		return 1;
	}
	if (sizeof value != 8)
	{
		return 2;
	}
	if (value.whole != 42U)
	{
		return 3;
	}
	value.byte = 9U;
	if (value.byte != 9U)
	{
		return 4;
	}
	if (initialized.whole != 27U)
	{
		return 5;
	}
	return 0;
}

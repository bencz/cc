typedef struct FunctionTable
{
	int (*apply)(int left, int right);
} FunctionTable;

static int add(int left, int right)
{
	return left + right;
}

static int subtract(int left, int right)
{
	return left - right;
}

FunctionTable operations = {add};

int main(void)
{
	int (*localOperation)(int left, int right) = subtract;

	if (operations.apply(7, 5) != 12)
	{
		return 1;
	}
	if (localOperation(7, 5) != 2)
	{
		return 2;
	}
	return 0;
}

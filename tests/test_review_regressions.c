static char text[4] = "abc";
static char *text_pointer = text;
static float global_single = 1.25f;
static char exact[3] = "abc";
static int twice(int value)
{
	return value * 2;
}

int main(void)
{
	char local[4] = "abc";
	int values[2] = {4, 7};
	int negative = -8;
	unsigned int count = 1U;
	float single = 1.5f;
	unsigned char narrow = 1;
	char exact_local[3] = "abc";
	double fraction = 0.5;
	if (local[0] != 'a' || local[2] != 'c' || local[3] != 0)
	{
		return 1;
	}
	if ((negative >> count) != -4 || (values)[1] != 7)
	{
		return 2;
	}
	if (text_pointer != text || twice(6) != 12)
	{
		return 3;
	}
	if (single != 1.5f || !(1 && fraction) || !(0 || fraction))
	{
		return 4;
	}
	if (global_single != 1.25f || exact[2] != 'c' || exact_local[2] != 'c' || !0.5 ||
	    ~narrow != -2 || -narrow != -1 || (1 + values) != &values[1])
	{
		return 5;
	}
	return 0;
}

#ifdef _WIN32
#define CALL WINAPI
#else
#define CALL
#endif

static int CALL add(int left, int right)
{
	return left + right;
}

int main(void)
{
	int index;
	int total = 0;
	for (index = 0; index < 200000; ++index)
	{
		total += add(1, 2);
	}
	return total != 600000;
}

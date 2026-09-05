typedef struct FirstDouble
{
	double value;
	char tag;
} FirstDouble;

typedef struct LaterDouble
{
	char tag;
	double value;
} LaterDouble;

typedef struct NestedDouble
{
	char tag;
	FirstDouble nested;
} NestedDouble;

int main(void)
{
	char bytes[sizeof(int) + 1];
	LaterDouble later;
	NestedDouble nested;
	int offset = (int)((char *)&later.value - (char *)&later);
	int nested_offset = (int)((char *)&nested.nested - (char *)&nested);
	if (sizeof(bytes) != 5 || sizeof(FirstDouble) != 16)
	{
		return 1;
	}
#ifdef _AIX
	if (offset != 4 || nested_offset != 4 || sizeof(LaterDouble) != 12 ||
	    sizeof(NestedDouble) != 20)
#else
	if (offset != 8 || nested_offset != 8 || sizeof(LaterDouble) != 16 ||
	    sizeof(NestedDouble) != 24)
#endif
	{
		return 2;
	}
	return 0;
}

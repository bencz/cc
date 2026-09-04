/* Pointer operations. */
int main()
{
	int x = 42;
	int *p = &x;
	int y = *p;
	int values[2] = {40, 2};
	int *element = values;
	element++;
	if (*element != 2)
	{
		return 1;
	}
	return y;
}

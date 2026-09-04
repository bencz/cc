typedef struct _Pair
{
	int first;
	int second;
} Pair;

typedef union _Number
{
	unsigned int whole;
	unsigned char byte;
} Number;

Pair global_pair = {.second = 8, .first = 3};
int global_array[5] = {[3] = 7, [1] = 2};
Number global_number = {.byte = 11};
int trailing_comma_array[] = {
    1,
    2,
};

int main(void)
{
	Pair pair = {.second = 6, .first = 4};
	int array[5] = {[4] = 9, [2] = 5};

	if (global_pair.first != 3 || global_pair.second != 8)
	{
		return 1;
	}
	if (global_array[0] != 0 || global_array[1] != 2 || global_array[2] != 0 ||
	    global_array[3] != 7 || global_array[4] != 0)
	{
		return 2;
	}
	if (global_number.byte != 11)
	{
		return 3;
	}
	if (pair.first != 4 || pair.second != 6)
	{
		return 4;
	}
	if (array[0] != 0 || array[1] != 0 || array[2] != 5 || array[3] != 0 || array[4] != 9)
	{
		return 5;
	}
	if (sizeof(trailing_comma_array) != 8 || trailing_comma_array[1] != 2)
	{
		return 6;
	}
	return 0;
}

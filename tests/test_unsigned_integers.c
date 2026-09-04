typedef unsigned int uint32;
typedef unsigned char *byte_pointer;

int main(void)
{
	unsigned int maximum = 0xffffffffU;
	signed int minus_one = -1;
	unsigned char byte = 255U;
	signed char signed_byte = 255U;
	short unsigned word = 65535U;
	long unsigned high_bit = 0x80000000UL;
	uint32 alias_value = 7U;
	unsigned int converted = (unsigned int)4294967295.0;
	double widened = (double)maximum;
	unsigned short words[3];
	byte_pointer pointer = 0;
	unsigned short counter = 1U;

	if (!(maximum > 1U))
	{
		return 1;
	}
	if (maximum / 2U != 0x7fffffffU)
	{
		return 2;
	}
	if (maximum % 7U != 3U)
	{
		return 3;
	}
	if ((maximum >> 31) != 1U)
	{
		return 4;
	}
	if ((minus_one >> 31) != -1)
	{
		return 5;
	}
	if (byte != 255)
	{
		return 6;
	}
	if (signed_byte != -1)
	{
		return 7;
	}
	if (word != 65535U)
	{
		return 8;
	}
	if (high_bit <= 1UL)
	{
		return 9;
	}
	if ((unsigned char)0x1ffU != 255U)
	{
		return 10;
	}
	if ((signed char)255U != -1)
	{
		return 11;
	}
	if (alias_value + 1U != 8U)
	{
		return 12;
	}
	if (converted != maximum)
	{
		return 13;
	}
	if (widened != 4294967295.0)
	{
		return 14;
	}
	if (maximum + 1.0 != 4294967296.0)
	{
		return 15;
	}
	if (1.0 + maximum != 4294967296.0)
	{
		return 16;
	}
	if (sizeof words != 6)
	{
		return 17;
	}
	if (sizeof(unsigned long) != 4)
	{
		return 18;
	}
	if (sizeof pointer != 4)
	{
		return 19;
	}
	counter++;
	if (counter != 2U)
	{
		return 20;
	}
	return 0;
}

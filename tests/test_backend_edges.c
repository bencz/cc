#include <limits.h>

typedef union FloatBits
{
	unsigned int bits;
	float value;
} FloatBits;

static char first_byte = 1;
static char second_byte = 2;
static char newline[] = "\n";

static int large_frame(int value)
{
	char bytes[70000];
	bytes[0] = 3;
	bytes[69999] = 7;
	return value + bytes[0] + bytes[69999];
}

int main(void)
{
	FloatBits bits;
	double nan;
	char byte = (char)255;
	bits.bits = 0x7fc00000U;
	nan = bits.value;
	if (nan == nan || nan < 0.0 || nan > 0.0 || nan <= 0.0 || nan >= 0.0 || !(nan != nan))
	{
		return 1;
	}
	if (large_frame(32) != 42 || first_byte + second_byte != 3)
	{
		return 2;
	}
#ifdef __CHAR_UNSIGNED__
	if (byte != 255 || CHAR_MIN != 0 || CHAR_MAX != 255)
#else
	if (byte != -1 || CHAR_MIN != -128 || CHAR_MAX != 127)
#endif
	{
		return 3;
	}
#ifdef __MVS__
	return newline[0] != 0x15;
#else
	return newline[0] != 10;
#endif
}

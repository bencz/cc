/* Bitwise operators. */
int main()
{
	int a = 0x0F;
	int b = 0xF0;
	int and_result = a & b;
	int or_result = a | b;
	int xor_result = a ^ b;
	int shift_left = a << 4;
	int shift_right = b >> 4;
	int tag = 3;
	void *packed = (void *)(0x04000000 | tag);
	if (((int)packed & 0x00FFFFFF) != 3)
	{
		return 1;
	}
	return and_result + or_result + xor_result + shift_left + shift_right;
}

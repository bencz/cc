int main(void)
{
	int octal = 010;
	int hex = 0x1e;
	double leading_dot = .5;
	double exponent = 1.25e+2;
	double hex_float = 0x1.8p+1;

	return octal == 8 && hex == 30 && leading_dot == 0.5 && exponent == 125.0 && hex_float == 3.0
	           ? 0
	           : 1;
}

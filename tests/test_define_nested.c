/* Nested macro expansion. */
#define A 10
#define B 20
#define C (A + B)
#define D (C * 2)

int main()
{
	int result = D;
	return result;
}

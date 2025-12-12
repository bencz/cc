/* test_float_binary.c - Converte float para representacao binaria */

char *strrev_t(char *str)
{
	char *p1;
	char *p2;
	int len;
	char temp;

	if (str == 0)
		return str;
	
	len = 0;
	while (str[len] != 0) len++;
	
	p1 = str;
	p2 = str + len - 1;
	
	while (p2 > p1)
	{
		temp = *p1;
		*p1 = *p2;
		*p2 = temp;
		p1++;
		p2--;
	}
	return str;
}

void floatToBinary(int ui, char *str, int numeroDeBits)
{
	int i;
	int strIndex;
	
	i = 0;
	strIndex = 0;

	while (i < numeroDeBits)
	{
		str[strIndex] = '0';
		strIndex++;
		i++;
	}

	i = 0;
	strIndex = 0;
	while (i < numeroDeBits)
	{
		if (ui & 1)
			str[strIndex] = '1';
		else
			str[strIndex] = '0';
		strIndex++;
		ui = ui >> 1;
		i++;
	}

  	str[strIndex] = 0;
	str = strrev_t(str);
}

int main()
{
	int input;
	int numeroDeBits;
	char str[33];
	
	input = 1078530011;
	numeroDeBits = 32;
	
	floatToBinary(input, str, numeroDeBits);
	
	return 0;
}

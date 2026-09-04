int ordinary_character = 'A';
int numeric_character = '\x41';
char ordinary_string[] = "A";
char numeric_string[] = "\x41";

int main(void)
{
	return ordinary_character == 65 && numeric_character == 65 && ordinary_string[0] == 65 &&
	               numeric_string[0] == 65
	           ? 0
	           : 1;
}

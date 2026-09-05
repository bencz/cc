static int value = 42;
static int *pointer = &value;

__declspec(dllexport) int WINAPI dll_value(int increment)
{
	return *pointer + increment;
}

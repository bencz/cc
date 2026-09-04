int this_external_function_name_is_deliberately_longer_than_fifty_x(void)
{
	return 7;
}

int main(void)
{
	return this_external_function_name_is_deliberately_longer_than_fifty_x() == 7 ? 0 : 1;
}

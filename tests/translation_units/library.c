static int private_helper(void)
{
    return 40;
}

static int private_value = 3;
int shared_initialized = 17;
int shared_zero;

int translation_unit_answer(void)
{
    return private_helper() + private_value + shared_initialized + shared_zero - 18;
}

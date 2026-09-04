int translation_unit_answer(void);
extern int shared_initialized;
extern int shared_zero;

static int private_helper(void)
{
    return 2;
}

int main(void)
{
    shared_zero = 1;
    return translation_unit_answer() + private_helper() + shared_initialized == 62 ? 0 : 1;
}

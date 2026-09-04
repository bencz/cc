#define FLAG 1
#if FLAG && ((3 * 4) == 12)
static int condition_value = 7;
#else
static int condition_value = 99;
#endif

#define QUOTE_ARGUMENT(x) "x"
#define JOINED_ADD(a, b) ((a) + (b))
#define EMPTY_ARGUMENT(x) (1 x)
#define PP_ADD(a, b) ((a) + (b))

#if defined(__STDC__) && defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
static int c99_predefined_macros = 16;
#else
static int c99_predefined_macros = 99;
#endif

#if PP_ADD(2, 3) == 5
static int function_macro_condition = 8;
#else
static int function_macro_condition = 99;
#endif

#if 0 && (1 / 0)
static int short_circuit_and = 99;
#else
static int short_circuit_and = 1;
#endif

#if 1 || (1 / 0)
static int short_circuit_or = 2;
#else
static int short_circuit_or = 99;
#endif

#if 1 ? 1 : (1 / 0)
static int short_circuit_conditional = 4;
#else
static int short_circuit_conditional = 99;
#endif

#if 0
#if (1 / 0)
static int inactive_nested_condition = 99;
#endif
#endif

int main(void)
{
	int /**/ value = condition_value;
	char *literal = QUOTE_ARGUMENT(replaced);
	return value == 7 && literal[0] == 'x' && literal[1] == 0 && short_circuit_and == 1 &&
	               short_circuit_or == 2 && short_circuit_conditional == 4 &&
	               JOINED_ADD(2, 3) == 5 && EMPTY_ARGUMENT() == 1 &&
	               function_macro_condition == 8 && c99_predefined_macros == 16
	           ? 0
	           : 1;
}

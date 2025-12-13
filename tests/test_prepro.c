/* Teste do preprocessador ISO C99 */

#define VERSION 100
#define EMPTY

#ifndef TEST_PREPRO_H
#define TEST_PREPRO_H

int test_ifdef = 1;

#endif

#ifdef VERSION
int has_version = VERSION;
#else
int has_version = 0;
#endif

#ifdef UNDEFINED_MACRO
int undefined = 1;
#else
int undefined = 0;
#endif

#if defined(VERSION)
int if_defined_works = 1;
#endif

#if !defined(UNDEFINED_MACRO)
int if_not_defined_works = 1;
#endif

#if 1
int if_one = 1;
#endif

#if 0
int if_zero_should_not_appear = 1;
#endif

/* Teste de macros com parâmetros */
#define SQUARE(x) ((x) * (x))
#define ADD(a, b) ((a) + (b))
#define MUL3(a, b, c) ((a) * (b) * (c))

int square_test = SQUARE(5);
int add_test = ADD(10, 20);
int mul3_test = MUL3(2, 3, 4);

/* Teste do operador # (stringify) */
#define STRINGIFY(x) #x

char *str_test = STRINGIFY(hello);

/* Teste do operador ## (concatenação) */
#define MAKE_VAR(n) var_ ## n

int MAKE_VAR(1) = 100;
int MAKE_VAR(2) = 200;

/* Teste de macros variádicas */
#define VARARGS_TEST(...) __VA_ARGS__

int varargs_result = VARARGS_TEST(1 + 2 + 3);

int main()
{
    int result = square_test + add_test + mul3_test;
    result = result + MAKE_VAR(1) + MAKE_VAR(2);
    result = result + varargs_result;
    return result;
}

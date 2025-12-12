/* test_include_limits.c - Teste de constantes de limites */
#define SHRT_MAX 32767
#define INT_MAX 2147483647

int main() {
    int max_short = SHRT_MAX;
    int max_int = INT_MAX;
    return (max_int > max_short) ? 1 : 0;
}

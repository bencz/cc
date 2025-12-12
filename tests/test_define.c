/* test_define.c - Teste de #define simples */
#define MAX_VALUE 100
#define MIN_VALUE 10
#define MULTIPLIER 2

int main() {
    int a = MAX_VALUE;
    int b = MIN_VALUE;
    int c = a + b;
    int d = c * MULTIPLIER;
    return d;
}

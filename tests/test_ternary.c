/* test_ternary.c - Teste de operador ternario */
int main() {
    int a = 10;
    int b = 20;
    int max = (a > b) ? a : b;
    int min = (a < b) ? a : b;
    return max + min;
}
